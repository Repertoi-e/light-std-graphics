module;

#include "lstd/platform/windows.h"

export module g.video.window.win32;

import g.video.window.general;
import g.video.monitor;

#if OS == WINDOWS
import lstd.path;
import lstd.os;
import lstd.fmt;
import lstd.signal;

// platform_is_windows_10_build_or_greater is defined in monitor.win32.cppm
#define IS_WINDOWS_10_ANNIVERSARY_UPDATE_OR_GREATER() platform_is_windows_10_build_or_greater(14393)
#define IS_WINDOWS_10_CREATORS_UPDATE_OR_GREATER() platform_is_windows_10_build_or_greater(15063)

//
// @ThreadSafety.
// Right now, functions dealing with windows must be called from the main thread only.
//

wchar* WindowClassName;

s32 AcquiredMonitorCount = 0;
u32 MouseTrailSize = 0;

window DisabledCursorWindow;
int2 RestoreCursorPos;

struct window_data;

window_data* WindowsList = null;
cursor* CursorsList = null;

bool MonitorCallbackAdded = false;

// We store this in
struct window_data {
	// We keep a global linked list of live windows
	window_data* Next = null;

	void* hWnd = null;
	window Handle;  // To avoid annoying casts... Has the same value as hWnd.

	void* BigIcon = null, * SmallIcon = null;

	bool CursorTracked = false;
	bool FrameAction = false;

	// See window.h
	u32 Flags = 0;

	s32 Width, Height;  // Filter out duplicate resize events

	u16 Surrogate = 0;  // Used when handling text input

	// The last received cursor position, regardless of source
	int2 LastCursorPos;

	// The state of each key (true if pressed)
	stack_array<bool, Key_Last + 1> Keys;
	stack_array<bool, Key_Last + 1> LastFrameKeys;  // Needed internally

	// The state of each key if it got changed this frame (true if pressed), use this to check for non-repeat
	stack_array<bool, Key_Last + 1> KeysThisFrame;

	// The state of the mouse buttons (true if clicked)
	stack_array<bool, Mouse_Button_Last + 1> MouseButtons;
	stack_array<bool, Mouse_Button_Last + 1> LastFrameMouseButtons;  // Needed internally

	// The state of each mouse button if it got changed this frame (true if clicked), use this to check for non-repeat
	stack_array<bool, Mouse_Button_Last + 1> MouseButtonsThisFrame;

	// _true_ when the window is closing
	bool IsDestroying = false;

	display_mode DisplayMode;
	monitor* Monitor = null;  // Non-null if we are fullscreen!
	cursor* Cursor = null;
	cursor_mode CursorMode = CURSOR_NORMAL;

	s32 AspectRatioNumerator = DONT_CARE, AspectRatioDenominator = DONT_CARE;

	// Min, max dimensions
	s32 MinW = DONT_CARE, MinH = DONT_CARE, MaxW = DONT_CARE, MaxH = DONT_CARE;

	// Virtual cursor position when cursor is disabled
	int2 VirtualCursorPos;

	// Enable raw (unscaled and unaccelerated) mouse motion when the cursor is disabled.
	// May not be supported on some platforms.
	bool RawMouseMotion = false;

	signal<bool(event)> Event;
};

file_scope window_data* get_window_data(window handle) { return (window_data*)GetWindowLongPtrW(handle, 0); }

file_scope auto MonitorCallback = [](const monitor_event& e) {
	if (e.Action == monitor_event::CONNECTED) return;

	auto* win = WindowsList;
	while (win) {
		if (win->Monitor == e.Monitor) {
			int2 size = get_size(win->Handle);
			set_fullscreen(win->Handle, null, size.x, size.y);
		}
		win = win->Next;
	}
};

file_scope void uninit(window handle);

file_scope DWORD get_window_style(window_data* win) {
	DWORD style = WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

	if (win->Monitor) {
		style |= WS_POPUP;
	}
	else {
		style |= WS_SYSMENU | WS_MINIMIZEBOX;

		if (win->Flags & BORDERLESS) {
			style |= WS_POPUP;

		}
		else {
			style |= WS_CAPTION;
			if (win->Flags & RESIZABLE) style |= WS_MAXIMIZEBOX | WS_THICKFRAME;
		}
	}
	return style;
}

file_scope DWORD get_window_ex_style(window_data* win) {
	DWORD style = WS_EX_APPWINDOW;
	if (win->Monitor || win->Flags & ALWAYS_ON_TOP) style |= WS_EX_TOPMOST;
	return style;
}

file_scope void update_framebuffer_transparency(window_data* win) {
	BOOL enabled;
	if (SUCCEEDED(DwmIsCompositionEnabled(&enabled)) && enabled) {
		HRGN region = CreateRectRgn(0, 0, -1, -1);

		DWM_BLURBEHIND bb = { 0 };
		bb.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
		bb.hRgnBlur = region;
		bb.fEnable = true;

		if (SUCCEEDED(DwmEnableBlurBehindWindow(win->hWnd, &bb))) {
			// Decorated windows don't repaint the transparent background leaving a trail behind animations.
			// Hack: Making the window layered with a transparency color key
			//       seems to fix this.  Normally, when specifying
			//       a transparency color key to be used when composing the
			//       layered window, all pixels painted by the window in this
			//       color will be transparent.  That doesn't seem to be the
			//       case anymore, at least when used with blur behind window
			//       plus negative region.
			LONG exStyle = GetWindowLongW(win->hWnd, GWL_EXSTYLE);
			exStyle |= WS_EX_LAYERED;

			(win->hWnd, GWL_EXSTYLE, exStyle);

			// Using a color key not equal to black to fix the trailing
			// issue.  When set to black, something is making the hit test
			// not resize with the window frame.
			SetLayeredWindowAttributes(win->hWnd, RGB(0, 193, 48), 255, LWA_COLORKEY);
		}
		DeleteObject(region);
	}
	else {
		LONG exStyle = GetWindowLongW(win->hWnd, GWL_EXSTYLE);
		exStyle &= ~WS_EX_LAYERED;
		SetWindowLongW(win->hWnd, GWL_EXSTYLE, exStyle);
		RedrawWindow(win->hWnd, null, null, RDW_ERASE | RDW_INVALIDATE | RDW_FRAME);
	}
}

file_scope int2 get_full_window_size(DWORD style, DWORD exStyle, s32 contentWidth, s32 contentHeight, u32 dpi) {
	RECT rect = { 0, 0, contentWidth, contentHeight };

	if (IS_WINDOWS_10_ANNIVERSARY_UPDATE_OR_GREATER()) {
		AdjustWindowRectExForDpi(&rect, style, false, exStyle, dpi);
	}
	else {
		AdjustWindowRectEx(&rect, style, false, exStyle);
	}
	return { rect.right - rect.left, rect.bottom - rect.top };
}

file_scope void do_key_input_event(window_data* win, u32 key, bool pressed, bool asyncMods = false) {
	assert(key <= Key_Last);

	if (!pressed && !win->Keys[key]) return;

	bool repeated = false;

	bool wasPressed = win->Keys[key];
	win->Keys[key] = pressed;
	if (pressed && wasPressed) repeated = true;

	event e;
	e.Window = win->Handle;
	if (pressed) {
		e.Type = repeated ? event::Key_Repeated : event::Key_Pressed;
	}
	else {
		e.Type = event::Key_Released;
	}
	e.KeyCode = key;
	emit_while_false(win->Event, e);
}

file_scope void do_mouse_input_event(window_data* win, u32 button, bool pressed, bool doubleClick = false) {
	assert(button <= Mouse_Button_Last);
	win->MouseButtons[button] = pressed;

	int2 pos = get_cursor_pos(win->Handle);

	event e;
	e.Window = win->Handle;
	e.Type = pressed ? event::Mouse_Button_Pressed : event::Mouse_Button_Released;
	e.Button = button;
	e.DoubleClicked = doubleClick;
	emit_while_false(win->Event, e);
}

file_scope void do_mouse_move(window_data* win, int2 pos) {
	if (win->VirtualCursorPos == pos) return;

	int2 delta = pos - win->VirtualCursorPos;
	win->VirtualCursorPos = pos;

	event e;
	e.Window = win->Handle;
	e.Type = event::Mouse_Moved;
	e.X = pos.x;
	e.Y = pos.y;
	e.DX = delta.x;
	e.DY = delta.y;
	emit_while_false(win->Event, e);
}

file_scope void acquire_monitor(window_data* win) {
	if (!AcquiredMonitorCount) {
		SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED);

		SystemParametersInfoW(SPI_GETMOUSETRAILS, 0, &MouseTrailSize, 0);
		SystemParametersInfoW(SPI_SETMOUSETRAILS, 0, 0, 0);
	}

	if (!win->Monitor->Window) ++AcquiredMonitorCount;

	set_display_mode(win->Monitor, win->DisplayMode);
	win->Monitor->Window = win->Handle;
}

// Remove the window and restore the original video mode
file_scope void release_monitor(window_data* win) {
	if (win->Monitor->Window != win->Handle) return;

	--AcquiredMonitorCount;
	if (!AcquiredMonitorCount) {
		SetThreadExecutionState(ES_CONTINUOUS);

		// Hack: Restore mouse trail length saved in acquireMonitor
		SystemParametersInfoW(SPI_SETMOUSETRAILS, MouseTrailSize, 0, 0);
	}

	win->Monitor->Window = null;
	restore_display_mode(win->Monitor);
}

file_scope void uninit(window handle) {
	if (!handle) return;

	auto* win = get_window_data(handle);
	win->IsDestroying = true;

	event e;
	e.Window = handle;
	e.Type = event::Window_Closed;
	emit_while_false(win->Event, e);

	if (win->Monitor) release_monitor(win);
	if (DisabledCursorWindow == handle) DisabledCursorWindow = null;

	if (handle) {
		RemovePropW(handle, L"LSTD");
		DestroyWindow(handle);
		handle = null;
	}

	if (win->BigIcon) DestroyIcon(win->BigIcon);
	if (win->SmallIcon) DestroyIcon(win->SmallIcon);

	free(win->Event);

	win->Handle = null;
}

file_scope void fit_to_monitor(window_data* win) {
	MONITORINFO mi = { sizeof(mi) };
	GetMonitorInfoW(win->Monitor->PlatformData.Win32.hMonitor, &mi);
	SetWindowPos(win->hWnd, HWND_TOPMOST, mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS);
}

// Creates an RGBA icon or cursor
file_scope HICON create_icon(bitmap* image, int xhot, int yhot, bool icon) {
	BITMAPV5HEADER bi;
	memset0(&bi, sizeof(bi));
	{
		bi.bV5Size = sizeof(bi);
		bi.bV5Width = image->Width;
		bi.bV5Height = -(s32)(image->Height);
		bi.bV5Planes = 1;
		bi.bV5BitCount = 32;
		bi.bV5Compression = BI_BITFIELDS;
		bi.bV5RedMask = 0x00ff0000;
		bi.bV5GreenMask = 0x0000ff00;
		bi.bV5BlueMask = 0x000000ff;
		bi.bV5AlphaMask = 0xff000000;
	}

	u8* target = null;

	HDC dc = GetDC(null);
	HBITMAP color = CreateDIBSection(dc, (BITMAPINFO*)&bi, DIB_RGB_COLORS, (void**)&target, null, (DWORD)0);
	ReleaseDC(null, dc);

	if (!color) {
		print(">>> {}:{} Failed to create RGBA bitmap.\n", __FILE__, __LINE__);
		return null;
	}
	defer(DeleteObject(color));

	HBITMAP mask = CreateBitmap(image->Width, image->Height, 1, 1, null);
	if (!mask) {
		print(">>> {}:{} Failed to create mask bitmap.\n", __FILE__, __LINE__);
		return null;
	}
	defer(DeleteObject(mask));

	u8* source = image->Pixels;
	For(range(image->Width * image->Height)) {
		target[0] = source[2];
		target[1] = source[1];
		target[2] = source[0];
		target[3] = source[3];
		target += 4;
		source += 4;
	}

	ICONINFO ii;
	memset0(&ii, sizeof(ii));
	{
		ii.fIcon = icon;
		ii.xHotspot = xhot;
		ii.yHotspot = yhot;
		ii.hbmMask = mask;
		ii.hbmColor = color;
	}

	HICON handle = CreateIconIndirect(&ii);
	if (!handle) {
		if (icon) {
			print(">>> {}:{} Failed to create RGBA icon.\n", __FILE__, __LINE__);
		}
		else {
			print(">>> {}:{} Failed to create RGBA cursor.\n", __FILE__, __LINE__);
		}
	}
	return handle;
}

file_scope s64 choose_icon(array<bitmap> icons, s32 width, s32 height) {
	s32 leastDiff = numeric<s32>::max();

	s64 closest = -1;
	For(range(icons.Count)) {
		auto icon = icons[it];
		s32 diff = abs((s32)(icon.Width * icon.Height - width * height));
		if (diff < leastDiff) {
			closest = it;
			leastDiff = diff;
		}
	}
	return closest;
}

// Updates the cursor clip rect
file_scope void update_clip_rect(window_data* win) {
	if (win) {
		RECT clipRect;
		GetClientRect(win->hWnd, &clipRect);
		ClientToScreen(win->hWnd, (POINT*)&clipRect.left);
		ClientToScreen(win->hWnd, (POINT*)&clipRect.right);
		ClipCursor(&clipRect);
	}
	else {
		ClipCursor(null);
	}
}

// Updates the cursor image according to its cursor mode
file_scope void update_cursor_image(window_data* win) {
	if (win->CursorMode == CURSOR_NORMAL) {
		if (win->Cursor) {
			SetCursor(win->Cursor->PlatformData.Win32.hCursor);
		}
		else {
			SetCursor(LoadCursorW(null, IDC_ARROW));
		}
	}
	else {
		SetCursor(null);  // We get here when the cursor mode is CURSOR_HIDDEN
	}
}

// Enables WM_INPUT messages for the mouse for the specified window
file_scope void enable_raw_mouse_motion(window_data* win) {
	RAWINPUTDEVICE rid = { 0x01, 0x02, 0, win->hWnd };
	if (!RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
		print(">>> {}:{} Failed to register raw input device. Raw mouse input may be unsupported.\n", __FILE__, __LINE__);
	}
}

// Disables WM_INPUT messages for the mouse
file_scope void disable_raw_mouse_motion() {
	RAWINPUTDEVICE rid = { 0x01, 0x02, RIDEV_REMOVE, null };
	if (!RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
		print(">>> {}:{} Failed to remove raw input device.\n", __FILE__, __LINE__);
	}
}

// Apply disabled cursor mode to a focused window
file_scope void disable_cursor(window_data* win) {
	DisabledCursorWindow = win->Handle;
	RestoreCursorPos = get_cursor_pos(win->Handle);
	update_cursor_image(win);
	set_cursor_pos(win->Handle, get_size(win->Handle) / 2);

	update_clip_rect(win);

	if (win->RawMouseMotion) enable_raw_mouse_motion(win);
}

// Exit disabled cursor mode for the specified window
file_scope void enable_cursor(window_data* win) {
	if (win->RawMouseMotion) disable_raw_mouse_motion();

	DisabledCursorWindow = null;
	update_clip_rect(null);
	set_cursor_pos(win->Handle, RestoreCursorPos);
	update_cursor_image(win);
}

file_scope void apply_aspect_ratio(window_data* win, s32 edge, RECT* area) {
	f32 ratio = (f32)win->AspectRatioNumerator / (f32)win->AspectRatioDenominator;

	u32 dpi = USER_DEFAULT_SCREEN_DPI;
	if (IS_WINDOWS_10_ANNIVERSARY_UPDATE_OR_GREATER()) dpi = GetDpiForWindow(win->hWnd);

	int2 off = get_full_window_size(get_window_style(win), get_window_ex_style(win), 0, 0, dpi);

	if (edge == WMSZ_LEFT || edge == WMSZ_BOTTOMLEFT || edge == WMSZ_RIGHT || edge == WMSZ_BOTTOMRIGHT) {
		area->bottom = area->top + off.y + (s32)((area->right - area->left - off.x) / ratio);
	}
	else if (edge == WMSZ_TOPLEFT || edge == WMSZ_TOPRIGHT) {
		area->top = area->bottom - off.y - (s32)((area->right - area->left - off.x) / ratio);
	}
	else if (edge == WMSZ_TOP || edge == WMSZ_BOTTOM) {
		area->right = area->left + off.x + (s32)((area->bottom - area->top - off.y) * ratio);
	}
}

file_scope void update_window_style(window_data* win) {
	DWORD style = GetWindowLongW(win->hWnd, GWL_STYLE);
	style &= ~(WS_OVERLAPPEDWINDOW | WS_POPUP);
	style |= get_window_style(win);

	RECT rect;
	GetClientRect(win->hWnd, &rect);

	if (IS_WINDOWS_10_ANNIVERSARY_UPDATE_OR_GREATER()) {
		AdjustWindowRectExForDpi(&rect, style, false, get_window_ex_style(win), GetDpiForWindow(win->hWnd));
	}
	else {
		AdjustWindowRectEx(&rect, style, false, get_window_ex_style(win));
	}

	ClientToScreen(win->hWnd, (POINT*)&rect.left);
	ClientToScreen(win->hWnd, (POINT*)&rect.right);
	SetWindowLongW(win->hWnd, GWL_STYLE, style);
	SetWindowPos(win->hWnd, HWND_TOP, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOZORDER);
}

file_scope u32 set_bit(u32 number, u32 mask, bool enabled) {
	return number & ~mask | mask & (enabled ? 0xFFFFFFFF : 0);
}

file_scope LRESULT __stdcall wnd_proc(HWND hWnd, u32 message, WPARAM wParam, LPARAM lParam) {
	auto* win = (window_data*)GetPropW(hWnd, L"LSTD");
	if (!win) {
		// This is the message handling for the hidden helper window
		// and for a regular window during its initial creation
		switch (message) {
		case WM_NCCREATE:
			if (IS_WINDOWS_10_ANNIVERSARY_UPDATE_OR_GREATER()) {
				// On per-monitor DPI aware V1 systems, only enable
				// non-client scaling for windows that scale the client area
				// We need WM_GETDPISCALEDSIZE from V2 to keep the client
				// area static when the non-client area is scaled.

				CREATESTRUCTW* cs = (CREATESTRUCTW*)lParam;
				bool scaleToMonitor = (u64)cs->lpCreateParams;
				if (scaleToMonitor) EnableNonClientDpiScaling(hWnd);
			}
			break;
		case WM_DISPLAYCHANGE:
			platform_poll_monitors();
			break;
		}
		return DefWindowProcW(hWnd, message, wParam, lParam);
	}

	assert(win->hWnd == hWnd);

	event e;
	e.Window = win->Handle;
	e.Type = event::Window_Platform_Message_Sent;
	e.Message = message;
	e.Param1 = wParam;
	e.Param2 = lParam;
	emit_while_false(win->Event, e);

	switch (message) {
	case WM_MOUSEACTIVATE:
		if (HIWORD(lParam) == WM_LBUTTONDOWN) {
			if (LOWORD(lParam) != HTCLIENT) win->FrameAction = true;
		}
		break;
	case WM_CAPTURECHANGED:
		// Hack: Disable the cursor once the caption button action has been completed or cancelled
		if (lParam == 0 && win->FrameAction) {
			if (win->CursorMode == CURSOR_DISABLED) disable_cursor(win);
			win->FrameAction = false;
		}

		break;
	case WM_SETFOCUS:
		win->Flags |= FOCUSED;

		{
			event e;
			e.Window = win->Handle;
			e.Type = event::Window_Focused;
			e.Focused = true;
			emit_while_false(win->Event, e);
		}

		// Hack: Do not disable cursor while the user is interacting with a caption button
		if (win->FrameAction) break;
		if (win->CursorMode == CURSOR_DISABLED) disable_cursor(win);
		return 0;
	case WM_KILLFOCUS:
		win->Flags &= ~FOCUSED;
		if (win->CursorMode == CURSOR_DISABLED) enable_cursor(win);
		if (win->Monitor && win->Flags & AUTO_MINIMIZE) minimize(win->Handle);

		{
			event e;
			e.Window = win->Handle;
			e.Type = event::Window_Focused;
			e.Focused = false;
			emit_while_false(win->Event, e);
		}

		For(range(Key_Last + 1)) {
			if (win->Keys[it]) do_key_input_event(win, (u32)it, false);
		}

		For(range(Mouse_Button_Last + 1)) {
			if (win->MouseButtons[it]) do_mouse_input_event(win, (u32)it, false);
		}
		return 0;
	case WM_SYSCOMMAND:
		switch (wParam & 0xfff0) {
		case SC_SCREENSAVE:
		case SC_MONITORPOWER: {
			if (win->Monitor) {
				// We are running in full screen mode, so disallow screen saver and screen blanking
				return 0;
			}
			else {
				break;
			}
		}
		case SC_KEYMENU:
			return 0;
		}
		break;
	case WM_CLOSE:
		win->IsDestroying = true;
		{
			event e;
			e.Window = win->Handle;
			e.Type = event::Window_Closed;
			emit_while_false(win->Event, e);
		}
		return 0;
	case WM_CHAR:
	case WM_UNICHAR: {
		if (message == WM_UNICHAR && wParam == UNICODE_NOCHAR) return true;

		auto cp = (code_point)wParam;
		if (cp < 32 || (cp > 126 && cp < 160)) return 0;

		if ((cp >= 0xD800) && (cp <= 0xDBFF)) {
			// First part of a surrogate pair: store it and wait for the second one
			win->Surrogate = (u16)cp;
		}
		else {
			if ((cp >= 0xDC00) && (cp <= 0xDFFF)) {
				cp = ((win->Surrogate - 0xD800) << 10) + (cp - 0xDC00) + 0x0010000;
				win->Surrogate = 0;
			}
			event e;
			e.Window = win->Handle;
			e.Type = event::Code_Point_Typed;
			e.CP = cp;
			emit_while_false(win->Event, e);
		}
		return 0;
	}
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYUP: {
		u32 key = (u32)wParam;
		if (key > 255) break;

		u32 keyHid = g_PlatformKeycodeNativeToHid[key];
		bool pressed = ((lParam >> 31) & 1) ? false : true;

		if (!pressed && wParam == VK_SHIFT) {
			// :ShiftHack: Release both Shift keys on Shift up event, as when both
			//             are pressed the first release does not emit any event
			// :ShiftHack: The other half of this is in update
			do_key_input_event(win, Key_LeftShift, false);
			do_key_input_event(win, Key_RightShift, false);
		}
		else if (wParam == VK_SNAPSHOT) {
			// Hack: Key down is not reported for the Print Screen key
			do_key_input_event(win, Key_PrintScreen, true);
			do_key_input_event(win, Key_PrintScreen, false);
		}
		else {
			do_key_input_event(win, keyHid, pressed);
		}

		if (win->Flags & CLOSE_ON_ALT_F4 && message == WM_SYSKEYDOWN && keyHid == Key_F4) {
			SendMessageW(win->hWnd, WM_CLOSE, 0, 0);
		}
		break;
	}
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_XBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDBLCLK:
	case WM_XBUTTONDBLCLK:
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
	case WM_XBUTTONUP: {
		u32 button;
		if (message == WM_LBUTTONDOWN || message == WM_LBUTTONUP || message == WM_LBUTTONDBLCLK) {
			button = Mouse_Button_Left;
		}
		else if (message == WM_RBUTTONDOWN || message == WM_RBUTTONUP || message == WM_RBUTTONDBLCLK) {
			button = Mouse_Button_Right;
		}
		else if (message == WM_MBUTTONDOWN || message == WM_MBUTTONUP || message == WM_MBUTTONDBLCLK) {
			button = Mouse_Button_Middle;
		}
		else if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) {
			button = Mouse_Button_X1;
		}
		else {
			button = Mouse_Button_X2;
		}

		bool pressed = message == WM_LBUTTONDOWN || message == WM_LBUTTONDBLCLK || message == WM_RBUTTONDOWN ||
			message == WM_RBUTTONDBLCLK || message == WM_MBUTTONDOWN || message == WM_MBUTTONDBLCLK ||
			message == WM_XBUTTONDOWN || message == WM_XBUTTONDBLCLK;

		bool anyPressed = false;
		For(range(Mouse_Button_Last + 1)) {
			if (win->MouseButtons[it]) {
				anyPressed = true;
				break;
			}
		}
		if (!anyPressed) SetCapture(hWnd);

		do_mouse_input_event(win, button, pressed, message == WM_LBUTTONDBLCLK || message == WM_RBUTTONDBLCLK || message == WM_MBUTTONDBLCLK || message == WM_XBUTTONDBLCLK);

		anyPressed = false;
		For(range(Mouse_Button_Last + 1)) {
			if (win->MouseButtons[it]) {
				anyPressed = true;
				break;
			}
		}
		if (!anyPressed) ReleaseCapture();

		if (message == WM_XBUTTONDOWN || message == WM_XBUTTONUP) return true;

		return 0;
	}
	case WM_MOUSEMOVE: {
		int2 pos = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

		if (!win->CursorTracked) {
			TRACKMOUSEEVENT tme;
			memset0(&tme, sizeof(tme));
			{
				tme.cbSize = sizeof(tme);
				tme.dwFlags = TME_LEAVE;
				tme.hwndTrack = win->hWnd;
			}
			TrackMouseEvent(&tme);

			win->CursorTracked = true;

			event e;
			e.Window = win->Handle;
			e.Type = event::Mouse_Entered_Window;
			emit_while_false(win->Event, e);
		}

		if (win->CursorMode == CURSOR_DISABLED) {
			if (DisabledCursorWindow != win->Handle) break;
			if (win->RawMouseMotion) break;

			int2 delta = pos - win->LastCursorPos;
			do_mouse_move(win, win->VirtualCursorPos + delta);
		}
		else {
			do_mouse_move(win, pos);
		}
		win->LastCursorPos = pos;

		return 0;
	}
	case WM_INPUT: {
		if (DisabledCursorWindow != win->Handle) break;
		if (!win->RawMouseMotion) break;

		HRAWINPUT ri = (HRAWINPUT)lParam;

		u32 size = 0;
		GetRawInputData(ri, RID_INPUT, null, &size, sizeof(RAWINPUTHEADER));

		auto* rawInput = malloc<RAWINPUT>({ .Count = size / sizeof(RAWINPUT), .Alloc = platform_get_temporary_allocator() });
		if (GetRawInputData(ri, RID_INPUT, rawInput, &size, sizeof(RAWINPUTHEADER)) == (u32)-1) {
			print(">>> {}:{} Failed to retrieve raw input data.\n", __FILE__, __LINE__);
			break;
		}

		s32 dx, dy;
		if (rawInput->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) {
			dx = rawInput->data.mouse.lLastX - win->LastCursorPos.x;
			dy = rawInput->data.mouse.lLastY - win->LastCursorPos.y;
		}
		else {
			dx = rawInput->data.mouse.lLastX;
			dy = rawInput->data.mouse.lLastY;
		}
		do_mouse_move(win, win->VirtualCursorPos + int2(dx, dy));

		win->LastCursorPos.x += dx;
		win->LastCursorPos.y += dy;
		break;
	}
	case WM_MOUSELEAVE:
		win->CursorTracked = false;
		{
			event e;
			e.Window = win->Handle;
			e.Type = event::Mouse_Left_Window;
			emit_while_false(win->Event, e);
		}
		return 0;
	case WM_MOUSEWHEEL: {
		event e;
		e.Window = win->Handle;
		e.Type = event::Mouse_Wheel_Scrolled;
		e.ScrollY = (u32)(GET_WHEEL_DELTA_WPARAM(wParam) / (f32)WHEEL_DELTA);
		emit_while_false(win->Event, e);
		return 0;
	}
	case WM_MOUSEHWHEEL:
		// NOTE: The X-axis is inverted for consistency with macOS and X11
	{
		event e;
		e.Window = win->Handle;
		e.Type = event::Mouse_Wheel_Scrolled;
		e.ScrollX = (u32)(-(GET_WHEEL_DELTA_WPARAM(wParam) / (f32)WHEEL_DELTA));
		emit_while_false(win->Event, e);
	}
	return 0;
	case WM_ENTERSIZEMOVE:
	case WM_ENTERMENULOOP:
		if (win->FrameAction) break;
		if (win->CursorMode == CURSOR_DISABLED) enable_cursor(win);
		break;
	case WM_EXITSIZEMOVE:
	case WM_EXITMENULOOP:
		if (win->FrameAction) break;
		if (win->CursorMode == CURSOR_DISABLED) disable_cursor(win);
		break;
	case WM_SIZE: {
		bool minimized = wParam == SIZE_MINIMIZED;
		bool maximized = wParam == SIZE_MAXIMIZED || (win->Flags & MAXIMIZED && wParam != SIZE_RESTORED);

		if (DisabledCursorWindow == win->Handle) update_clip_rect(win);

		bool windowMinimized = win->Flags & MINIMIZED;
		bool windowMaximized = win->Flags & MAXIMIZED;

		if (minimized && !windowMinimized) {
			win->Flags |= MINIMIZED;
			minimize(win->Handle);

			event e;
			e.Window = win->Handle;
			e.Type = event::Window_Minimized;
			e.Minimized = true;
			emit_while_false(win->Event, e);
		}
		if (windowMinimized && !minimized) {
			win->Flags &= ~MINIMIZED;

			event e;
			e.Window = win->Handle;
			e.Type = event::Window_Minimized;
			e.Minimized = false;
			emit_while_false(win->Event, e);
		}

		if (maximized && !windowMaximized) {
			win->Flags |= MAXIMIZED;
			maximize(win->Handle);

			event e;
			e.Window = win->Handle;
			e.Type = event::Window_Maximized;
			e.Maximized = true;
			emit_while_false(win->Event, e);
		}
		if (windowMaximized && !maximized) {
			win->Flags &= ~MAXIMIZED;

			event e;
			e.Window = win->Handle;
			e.Type = event::Window_Maximized;
			e.Maximized = false;
			emit_while_false(win->Event, e);
		}

		s32 newWidth = LOWORD(lParam);
		s32 newHeight = HIWORD(lParam);

		if (newWidth != win->Width || newHeight != win->Height) {
			event e;
			e.Window = win->Handle;
			e.Type = event::Window_Framebuffer_Resized;
			e.Width = newWidth;
			e.Height = newHeight;

			emit_while_false(win->Event, e);
			e.Type = event::Window_Resized;
			emit_while_false(win->Event, e);

			win->Width = newWidth;
			win->Height = newHeight;
		}

		if (win->Monitor && windowMinimized != minimized) {
			if (minimized) {
				release_monitor(win);
			}
			else {
				acquire_monitor(win);
				fit_to_monitor(win);
			}
		}
		return 0;
	}
	case WM_SHOWWINDOW:
		win->Flags = set_bit(win->Flags, SHOWN, (bool)wParam);
		win->Flags = set_bit(win->Flags, HIDDEN, (bool)!wParam);
		break;
	case WM_MOVE:
		if (DisabledCursorWindow == win->Handle) update_clip_rect(win);
		{
			event e;
			e.Window = win->Handle;
			e.Type = event::Window_Moved;
			e.X = GET_X_LPARAM(lParam);
			e.Y = GET_Y_LPARAM(lParam);
			emit_while_false(win->Event, e);
		}
		return 0;

	case WM_SIZING:
		if (win->AspectRatioNumerator == DONT_CARE || win->AspectRatioDenominator == DONT_CARE) {
			break;
		}
		apply_aspect_ratio(win, (s32)wParam, (RECT*)lParam);
		return true;
	case WM_GETMINMAXINFO: {
		if (win->Monitor) break;

		UINT dpi = USER_DEFAULT_SCREEN_DPI;
		if (IS_WINDOWS_10_ANNIVERSARY_UPDATE_OR_GREATER()) dpi = GetDpiForWindow(win->hWnd);

		auto* mmi = (MINMAXINFO*)lParam;

		int2 off = get_full_window_size(get_window_style(win), get_window_ex_style(win), 0, 0, dpi);
		if (win->MinH != DONT_CARE) mmi->ptMinTrackSize.x = win->MinW + off.x;
		if (win->MinW != DONT_CARE) mmi->ptMinTrackSize.y = win->MinH + off.y;

		if (win->MaxW != DONT_CARE) mmi->ptMaxTrackSize.x = win->MaxW + off.x;
		if (win->MinW != DONT_CARE) mmi->ptMaxTrackSize.y = win->MaxH + off.y;

		if (win->Flags & BORDERLESS) {
			HMONITOR mh = MonitorFromWindow(win->hWnd, MONITOR_DEFAULTTONEAREST);

			MONITORINFO mi;
			memset0(&mi, sizeof(mi));
			mi.cbSize = sizeof(mi);
			GetMonitorInfoW(mh, &mi);

			mmi->ptMaxPosition.x = mi.rcWork.left - mi.rcMonitor.left;
			mmi->ptMaxPosition.y = mi.rcWork.top - mi.rcMonitor.top;
			mmi->ptMaxSize.x = mi.rcWork.right - mi.rcWork.left;
			mmi->ptMaxSize.y = mi.rcWork.bottom - mi.rcWork.top;
		}
		return 0;
	}
	case WM_PAINT: {
		event e;
		e.Window = win->Handle;
		e.Type = event::Window_Refreshed;
		emit_while_false(win->Event, e);
	} break;
	case WM_ERASEBKGND:
		return true;
	case WM_NCACTIVATE:
	case WM_NCPAINT:
		// Prevent title bar from being drawn after restoring a minimized undecorated window
		if (win->Flags & BORDERLESS) return true;
		break;
	case WM_NCHITTEST:
		if (win->Flags & MOUSE_PASS_THROUGH) return HTTRANSPARENT;
		break;
	case WM_GETDPISCALEDSIZE: {
		if (!(win->Flags & SCALE_TO_MONITOR))
			break;

		// Adjust the window size to keep the content area size constant
		if (IS_WINDOWS_10_CREATORS_UPDATE_OR_GREATER()) {
			RECT source{};
			RECT target{};
			SIZE* size = (SIZE*)lParam;

			AdjustWindowRectExForDpi(&source, get_window_style(win), false, get_window_ex_style(win), GetDpiForWindow(win->hWnd));
			AdjustWindowRectExForDpi(&target, get_window_style(win), false, get_window_ex_style(win), LOWORD(wParam));

			size->cx += (target.right - target.left) - (source.right - source.left);
			size->cy += (target.bottom - target.top) - (source.bottom - source.top);
			return true;
		}
		break;
	}
	case WM_DPICHANGED: {
		f32 xscale = HIWORD(wParam) / (f32)USER_DEFAULT_SCREEN_DPI;
		f32 yscale = LOWORD(wParam) / (f32)USER_DEFAULT_SCREEN_DPI;

		// Only apply the suggested size if the OS is new enough to have
		// sent a WM_GETDPISCALEDSIZE before this
		if (!win->Monitor && (win->Flags & SCALE_TO_MONITOR || IS_WINDOWS_10_CREATORS_UPDATE_OR_GREATER())) {
			RECT* suggested = (RECT*)lParam;
			SetWindowPos(win->hWnd, HWND_TOP, suggested->left, suggested->top, suggested->right - suggested->left, suggested->bottom - suggested->top, SWP_NOACTIVATE | SWP_NOZORDER);
		}
		event e;
		e.Window = win->Handle;
		e.Type = event::Window_Content_Scale_Changed;
		e.Scale = { xscale, yscale };
		emit_while_false(win->Event, e);
		break;
	}
	case WM_THEMECHANGED:
	case WM_SETTINGCHANGE:
	case WM_DWMCOMPOSITIONCHANGED:
		// @TODO: Handle font size change here!

		InvalidateRect(hWnd, null, true);

		if (message == WM_DWMCOMPOSITIONCHANGED) {
			if (win->Flags & ALPHA) update_framebuffer_transparency(win);
			return 0;
		}
		break;
	case WM_SETCURSOR:
		if (LOWORD(lParam) == HTCLIENT) {
			update_cursor_image(win);
			return true;
		}
		break;
	case WM_DROPFILES: {
		HDROP drop = (HDROP)wParam;

		// Move the mouse to the position of the drop
		POINT pt;
		DragQueryPoint(drop, &pt);
		do_mouse_move(win, { pt.x, pt.y });

		s32 count = DragQueryFileW(drop, 0xffffffff, null, 0);

		array<string> paths;
		reserve(paths, count, platform_get_persistent_allocator());

		For(range(count)) {
			u32 length = DragQueryFileW(drop, (u32)it, null, 0);

			wchar* buffer = malloc<wchar>({ .Count = length + 1, .Alloc = platform_get_temporary_allocator() });
			DragQueryFileW(drop, (u32)it, buffer, length + 1);

			add(paths, platform_utf16_to_utf8(buffer, platform_get_persistent_allocator()));
		}

		event e;
		e.Window = win->Handle;
		e.Type = event::Window_Files_Dropped;
		e.Paths = paths;
		emit_while_false(win->Event, e);

		For(paths) free(it.Data);
		free(paths.Data);

		DragFinish(drop);
		return 0;
	}
	}
	return DefWindowProcW(hWnd, message, wParam, lParam);
}

// This needs to be handled by the runtime
export {
	// :BeforeMain:
	void platform_window_init() {
		GUID guid;
		WIN32_CHECK_HR(r1, CoCreateGuid(&guid));
		WIN32_CHECK_HR(r2, StringFromCLSID(guid, &WindowClassName));

		WNDCLASSEXW wc;
		memset0(&wc, sizeof(wc));
		{
			wc.cbSize = sizeof(wc);
			wc.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
			wc.lpfnWndProc = wnd_proc;
			wc.hInstance = GetModuleHandleW(null);
			wc.hCursor = LoadCursorW(null, IDC_ARROW);
			wc.lpszClassName = WindowClassName;
			wc.cbWndExtra = sizeof(window_data*);

			// Load user-provided icon if available
			wc.hIcon = (HICON)LoadImageW(GetModuleHandleW(null), L"WINDOW ICON", IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
			if (!wc.hIcon) {
				// No user-provided icon found, load default icon
				wc.hIcon = (HICON)LoadImageW(null, IDI_APPLICATION, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
			}
		}

		if (!RegisterClassExW(&wc)) {
			print(">>> {}:{} Failed to register window class.\n", __FILE__, __LINE__);
			assert(false);
		}
	}

	// :AfterMain:
	void platform_window_uninit() {
		// Destroy all left-over windows
		auto* win = WindowsList;
		while (win) {
			auto* next = win->Next;
			uninit(win->Handle);
			free(win);
			win = next;
		}
	}
}

export {
	void set_forced_aspect_ratio(window w, s32 numerator, s32 denominator) {
		auto win = get_window_data(w);

		if (numerator != DONT_CARE && denominator != DONT_CARE) {
			if (numerator <= 0 || denominator <= 0) {
				print(">>> {}:{} Invalid window aspect ratio ({}:{}).\n", __FILE__, __LINE__, numerator, denominator);
				return;
			}
		}

		win->AspectRatioNumerator = numerator;
		win->AspectRatioDenominator = denominator;

		if (numerator == DONT_CARE || denominator == DONT_CARE) return;

		RECT area;
		GetWindowRect(win->hWnd, &area);
		apply_aspect_ratio(win, WMSZ_BOTTOMRIGHT, &area);
		MoveWindow(win->hWnd, area.left, area.top, area.right - area.left, area.bottom - area.top, true);
	}

	void set_raw_mouse(window w, bool enabled) {
		auto win = get_window_data(w);

		if (win->RawMouseMotion == enabled) return;

		if (DisabledCursorWindow != w) {
			win->RawMouseMotion = enabled;
			if (enabled) {
				enable_raw_mouse_motion(win);
			}
			else {
				disable_raw_mouse_motion();
			}
		}
	}

	void set_cursor_mode(window w, cursor_mode mode) {
		auto win = get_window_data(w);

		if (win->CursorMode == mode) return;

		win->CursorMode = mode;
		win->VirtualCursorPos = get_cursor_pos(w);

		if (mode == CURSOR_DISABLED) {
			if (win->Flags & FOCUSED) disable_cursor(win);
		}
		else if (DisabledCursorWindow == w) {
			enable_cursor(win);
		}
		else if (is_hovered(w)) {
			update_cursor_image(win);
		}
	}

	cursor_mode get_cursor_mode(window w) {
		auto win = get_window_data(w);
		return win->CursorMode;
	}

	f32 get_opacity(window w) {
		auto win = get_window_data(w);

		BYTE alpha;
		DWORD flags;

		if ((GetWindowLongW(win->hWnd, GWL_EXSTYLE) & WS_EX_LAYERED) &&
			GetLayeredWindowAttributes(win->hWnd, null, &alpha, &flags)) {
			if (flags & LWA_ALPHA) return alpha / 255.f;
		}
		return 1.f;
	}

	void set_opacity(window w, f32 opacity) {
		auto win = get_window_data(w);

		assert(opacity >= 0 && opacity <= 1.0f);

		if (opacity < 1.0f) {
			BYTE alpha = (BYTE)(255 * opacity);
			DWORD style = GetWindowLongW(win->hWnd, GWL_EXSTYLE);
			style |= WS_EX_LAYERED;
			SetWindowLongW(win->hWnd, GWL_EXSTYLE, style);
			SetLayeredWindowAttributes(win->hWnd, 0, alpha, LWA_ALPHA);
		}
		else {
			DWORD style = GetWindowLongW(win->hWnd, GWL_EXSTYLE);
			style &= ~WS_EX_LAYERED;
			SetWindowLongW(win->hWnd, GWL_EXSTYLE, style);
		}
	}

	void set_borderless(window w, bool enabled) {
		auto win = get_window_data(w);

		win->Flags = set_bit(win->Flags, BORDERLESS, enabled);
		if (!win->Monitor) update_window_style(win);
	}

	void set_resizable(window w, bool enabled) {
		auto win = get_window_data(w);

		win->Flags = set_bit(win->Flags, RESIZABLE, enabled);
		if (!win->Monitor) update_window_style(win);
	}

	void set_always_on_top(window w, bool enabled) {
		auto win = get_window_data(w);

		win->Flags = set_bit(win->Flags, ALWAYS_ON_TOP, enabled);
		if (!win->Monitor) {
			HWND after = enabled ? HWND_TOPMOST : HWND_NOTOPMOST;
			SetWindowPos(win->hWnd, after, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
		}
	}

	void set_auto_minimize(window w, bool enabled) {
		auto win = get_window_data(w);
		win->Flags = set_bit(win->Flags, AUTO_MINIMIZE, enabled);
	}

	void set_focus_on_show(window w, bool enabled) {
		auto win = get_window_data(w);
		win->Flags = set_bit(win->Flags, FOCUS_ON_SHOW, enabled);
	}

	void set_vsync(window w, bool enabled) {
		auto win = get_window_data(w);
		win->Flags = set_bit(win->Flags, VSYNC, enabled);
	}

	void set_close_on_alt_f4(window w, bool enabled) {
		auto win = get_window_data(w);
		win->Flags = set_bit(win->Flags, CLOSE_ON_ALT_F4, enabled);
	}

	void set_mouse_pass_through(window w, bool enabled) {
		auto win = get_window_data(w);
		win->Flags = set_bit(win->Flags, MOUSE_PASS_THROUGH, enabled);
	}

	bool is_hovered(window w) {
		auto win = get_window_data(w);

		POINT pos;
		if (!GetCursorPos(&pos)) return false;
		if (WindowFromPoint(pos) != win->hWnd) return false;

		RECT area;
		GetClientRect(win->hWnd, &area);
		ClientToScreen(win->hWnd, (POINT*)&area.left);
		ClientToScreen(win->hWnd, (POINT*)&area.right);

		return PtInRect(&area, pos);
	}

	bool is_visible(window w) {
		auto win = get_window_data(w);

		if ((win->Flags & MINIMIZED) || (win->Flags & HIDDEN)) return false;

		int2 s = get_size(w);
		if (s.x == 0 || s.y == 0) return false;
		return true;
	}

	void show(window w) {
		auto win = get_window_data(w);

		ShowWindow(win->hWnd, SW_SHOWNA);
		if (win->Flags & FOCUS_ON_SHOW) focus(w);
	}

	void hide(window w) {
		auto win = get_window_data(w);
		ShowWindow(win->hWnd, SW_HIDE);
	}

	void minimize(window w) {
		auto win = get_window_data(w);
		ShowWindow(win->hWnd, SW_MINIMIZE);
	}

	void restore(window w) {
		auto win = get_window_data(w);
		ShowWindow(win->hWnd, SW_RESTORE);
	}

	void maximize(window w) {
		auto win = get_window_data(w);
		ShowWindow(win->hWnd, SW_MAXIMIZE);
	}

	void focus(window w) {
		auto win = get_window_data(w);

		BringWindowToTop(win->hWnd);
		SetForegroundWindow(win->hWnd);
		SetFocus(win->hWnd);
	}

	void request_attention(window w) {
		auto win = get_window_data(w);
		FlashWindow(win->hWnd, true);
	}

	window create_window(string title, s32 x, s32 y, s32 width, s32 height, u32 flags) {
		auto* win = malloc<window_data>();

		if (!MonitorCallbackAdded) {
			monitor_connect_callback(&MonitorCallback);
			MonitorCallbackAdded = true;
		}

		win->DisplayMode.Width = width;
		win->DisplayMode.Height = height;
		win->DisplayMode.RedBits = win->DisplayMode.GreenBits = win->DisplayMode.BlueBits = 8;

		win->DisplayMode.RefreshRate = DONT_CARE;

		win->Flags = flags & CREATION_FLAGS;

		DWORD style = get_window_style(win);
		DWORD exStyle = get_window_ex_style(win);

		int2 fullSize = get_full_window_size(style, exStyle, width, height, USER_DEFAULT_SCREEN_DPI);

		s32 xpos = x == DONT_CARE ? CW_USEDEFAULT : x;
		s32 ypos = y == DONT_CARE ? CW_USEDEFAULT : y;
		if (x == CENTERED) xpos = (get_primary_monitor()->CurrentMode.Width - fullSize.x) / 2;
		if (y == CENTERED) ypos = (get_primary_monitor()->CurrentMode.Height - fullSize.y) / 2;

		win->hWnd = CreateWindowExW(exStyle, WindowClassName, L"", style, xpos, ypos, fullSize.x, fullSize.y, null, null, GetModuleHandleW(null), (void*)(u64)(win->Flags & SCALE_TO_MONITOR));

		if (!win->hWnd) {
			print(">>> {}:{} Failed to create window.\n", __FILE__, __LINE__);
			free(win);
			return null;
		}

		window handle = win->hWnd;
		win->Handle = handle;

		SetWindowLongPtrW(handle, 0, (LONG_PTR)win);

		win->Next = WindowsList;
		WindowsList = win;

		set_title(handle, title);

		// Adjust window rect to account for DPI scaling of the window frame and
		// (if enabled) DPI scaling of the content area. This cannot be done until
		// we know what monitor the window was placed on
		RECT rect = { 0, 0, width, height };

		if (win->Flags & SCALE_TO_MONITOR) {
			float2 scale = get_content_scale(monitor_from_window(handle));
			rect.right = (s32)(rect.right * scale.x);
			rect.bottom = (s32)(rect.bottom * scale.y);
		}

		ClientToScreen(handle, (POINT*)&rect.left);
		ClientToScreen(handle, (POINT*)&rect.right);

		if (IS_WINDOWS_10_ANNIVERSARY_UPDATE_OR_GREATER()) {
			AdjustWindowRectExForDpi(&rect, style, false, exStyle, GetDpiForWindow(handle));
		}
		else {
			AdjustWindowRectEx(&rect, style, false, exStyle);
		}

		// Only update the restored window rect as the window may be maximized
		WINDOWPLACEMENT wp = { sizeof(wp) };
		GetWindowPlacement(handle, &wp);
		wp.rcNormalPosition = rect;
		wp.showCmd = SW_HIDE;
		SetWindowPlacement(handle, &wp);

		SetPropW(handle, L"LSTD", win);

		ChangeWindowMessageFilterEx(handle, WM_DROPFILES, MSGFLT_ALLOW, null);
		ChangeWindowMessageFilterEx(handle, WM_COPYDATA, MSGFLT_ALLOW, null);
		ChangeWindowMessageFilterEx(handle, WM_COPYGLOBALDATA, MSGFLT_ALLOW, null);
		DragAcceptFiles(handle, true);

		if (win->Flags & ALPHA) update_framebuffer_transparency(win);
		if (win->Flags & SHOWN) show(handle);

		// If we couldn't get transparent, remove the flag
		BOOL enabled;
		if (FAILED(DwmIsCompositionEnabled(&enabled)) || !enabled) {
			win->Flags &= ~ALPHA;
		}

		auto s = get_size(handle);
		win->Width = s.x;
		win->Height = s.y;

		memset0(win->Keys.Data, sizeof(win->Keys.Data));
		memset0(win->LastFrameKeys.Data, sizeof(win->LastFrameKeys.Data));
		memset0(win->KeysThisFrame.Data, sizeof(win->KeysThisFrame.Data));
		memset0(win->MouseButtons.Data, sizeof(win->MouseButtons.Data));
		memset0(win->LastFrameMouseButtons.Data, sizeof(win->LastFrameMouseButtons.Data));
		memset0(win->MouseButtonsThisFrame.Data, sizeof(win->MouseButtonsThisFrame.Data));

		return handle;
	}

	s64 connect_event(window w, delegate<bool(event)> sb) {
		auto win = get_window_data(w);
		return connect(win->Event, sb);
	}

	bool disconnect_event(window w, s64 cb) {
		auto win = get_window_data(w);
		return disconnect(win->Event, cb);
	}

	u32 get_flags(window w) {
		auto win = get_window_data(w);
		return win->Flags;
	}

	bool is_destroying(window w) {
		auto win = get_window_data(w);
		return win->IsDestroying;
	}

	void update_windows() {
		MSG message;
		while (PeekMessageW(&message, null, 0, 0, PM_REMOVE) > 0) {
			if (message.message == WM_QUIT) {
				auto* win = WindowsList;
				while (win) {
					auto* next = win->Next;
					uninit(win->Handle);
					free(win);
					win = next;
				}
			}
			else {
				TranslateMessage(&message);
				DispatchMessageW(&message);
			}
		}

		HWND handle = GetActiveWindow();
		if (handle) {
			// :ShiftHack: Shift keys on Windows tend to "stick" when both are pressed as
			//             no key up message is generated by the first key release
			//             The other half of this is in the handling of WM_KEYUP
			// :ShiftHack: The other half of this is in WM_SYSKEYUP
			auto* win = (window_data*)GetPropW(handle, L"LSTD");
			if (win) {
				bool lshift = (GetAsyncKeyState(VK_LSHIFT) >> 15) & 1;
				bool rshift = (GetAsyncKeyState(VK_RSHIFT) >> 15) & 1;

				if (!lshift && win->Keys[Key_LeftShift]) {
					do_key_input_event(win, Key_LeftShift, false, true);
				}
				else if (!rshift && win->Keys[Key_RightShift]) {
					do_key_input_event(win, Key_RightShift, false, true);
				}
			}
		}

		auto* win = WindowsList;
		while (win) {
			For(range(Key_Last + 1)) { win->KeysThisFrame[it] = win->Keys[it] && !win->LastFrameKeys[it]; }
			For(range(Mouse_Button_Last + 1)) {
				win->MouseButtonsThisFrame[it] = win->MouseButtons[it] && !win->LastFrameMouseButtons[it];
			}
			win->LastFrameKeys = win->Keys;
			win->LastFrameMouseButtons = win->MouseButtons;

			win = win->Next;
		}

		if (DisabledCursorWindow) {
			int2 size = get_size(DisabledCursorWindow);
			if (get_window_data(DisabledCursorWindow)->LastCursorPos != size / 2) {
				set_cursor_pos(DisabledCursorWindow, size / 2);
			}
		}
	}

	void free_window(window handle) {
		auto* win = get_window_data(handle);

		uninit(handle);

		window_data** prev = &WindowsList;
		while (*prev != win) prev = &((*prev)->Next);
		*prev = win->Next;

		free(win);
	}

	string get_title(window w) {
		auto* win = get_window_data(w);

		s32 length = GetWindowTextLengthW(win->hWnd) + 1;

		auto* titleUtf16 = malloc<wchar>({ .Count = length, .Alloc = platform_get_temporary_allocator() });
		defer(free(titleUtf16));

		GetWindowTextW(win->hWnd, titleUtf16, length);

		return platform_utf16_to_utf8(titleUtf16, platform_get_persistent_allocator());
	}

	void set_title(window w, string title) {
		auto* win = get_window_data(w);

		// title.Length * 2 because one unicode character might take 2 wide chars.
		// This is just an approximation, not all space will be used!
		//auto *titleUtf16 = malloc<wchar>({.Count = title.Length * 2 + 1, .Alloc = platform_get_temporary_allocator()});

		auto* titleUtf16 = platform_utf8_to_utf16(title);
		SetWindowTextW(win->hWnd, titleUtf16);
	}



	void set_fullscreen(window w, monitor* mon, s32 width, s32 height, s32 refreshRate) {
		auto* win = get_window_data(w);

		win->DisplayMode.Width = width;
		win->DisplayMode.Height = height;
		win->DisplayMode.RefreshRate = refreshRate;

		if (win->Monitor == mon) {
			if (mon) {
				if (mon->Window == w) {
					acquire_monitor(win);
					fit_to_monitor(win);
				}
			}
			else {
				RECT rect = { 0, 0, width, height };

				if (IS_WINDOWS_10_ANNIVERSARY_UPDATE_OR_GREATER()) {
					AdjustWindowRectExForDpi(&rect, get_window_style(win), false, get_window_ex_style(win), GetDpiForWindow(win->hWnd));
				}
				else {
					AdjustWindowRectEx(&rect, get_window_style(win), false, get_window_ex_style(win));
				}

				SetWindowPos(win->hWnd, HWND_TOP, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_NOCOPYBITS | SWP_NOACTIVATE | SWP_NOZORDER);
			}
			return;
		}

		if (win->Monitor) release_monitor(win);
		win->Monitor = mon;

		if (win->Monitor) {
			u32 flags = SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOCOPYBITS;
			if (!(win->Flags & BORDERLESS)) {
				DWORD style = GetWindowLongW(win->hWnd, GWL_STYLE);
				style &= ~WS_OVERLAPPEDWINDOW;
				style |= get_window_style(win);
				SetWindowLongW(win->hWnd, GWL_STYLE, style);

				flags |= SWP_FRAMECHANGED;
			}

			acquire_monitor(win);

			MONITORINFO mi = { sizeof(mi) };
			GetMonitorInfoW(win->Monitor->PlatformData.Win32.hMonitor, &mi);
			SetWindowPos(win->hWnd, HWND_TOPMOST, mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top, flags);
		}
		else {
			UINT flags = SWP_NOACTIVATE | SWP_NOCOPYBITS;
			if (!(win->Flags & BORDERLESS)) {
				DWORD style = GetWindowLongW(win->hWnd, GWL_STYLE);
				style &= ~WS_POPUP;
				style |= get_window_style(win);
				SetWindowLongW(win->hWnd, GWL_STYLE, style);

				flags |= SWP_FRAMECHANGED;
			}

			HWND after;
			if (win->Flags & ALWAYS_ON_TOP) {
				after = HWND_TOPMOST;
			}
			else {
				after = HWND_NOTOPMOST;
			}

			RECT rect = { 0, 0, width, height };
			if (IS_WINDOWS_10_ANNIVERSARY_UPDATE_OR_GREATER()) {
				AdjustWindowRectExForDpi(&rect, get_window_style(win), false, get_window_ex_style(win), GetDpiForWindow(win->hWnd));
			}
			else {
				AdjustWindowRectEx(&rect, get_window_style(win), false, get_window_ex_style(win));
			}

			SetWindowPos(win->hWnd, after, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, flags);
		}
	}

	bool is_fullscreen(window w) {
		auto win = get_window_data(w);
		return win->Monitor;
	}

	void set_icon(window w, array<bitmap> icons) {
		auto win = get_window_data(w);

		HICON bigIcon = null, smallIcon = null;

		if (icons.Count) {
			s64 closestBig = choose_icon(icons, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON));
			s64 closestSmall = choose_icon(icons, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON));

			bigIcon = create_icon(icons.Data + closestBig, 0, 0, true);
			smallIcon = create_icon(icons.Data + closestSmall, 0, 0, true);
		}
		else {
			bigIcon = (HICON)GetClassLongPtrW(win->hWnd, GCLP_HICON);
			smallIcon = (HICON)GetClassLongPtrW(win->hWnd, GCLP_HICONSM);
		}

		SendMessageW(win->hWnd, WM_SETICON, ICON_BIG, (LPARAM)bigIcon);
		SendMessageW(win->hWnd, WM_SETICON, ICON_SMALL, (LPARAM)smallIcon);

		if (win->BigIcon) DestroyIcon(win->BigIcon);
		if (win->SmallIcon) DestroyIcon(win->SmallIcon);

		if (icons.Count) {
			win->BigIcon = bigIcon;
			win->SmallIcon = smallIcon;
		}
	}


	void set_cursor(window w, cursor* curs) {
		auto win = get_window_data(w);

		win->Cursor = curs;
		if (is_hovered(w)) update_cursor_image(win);
	}

	int2 get_cursor_pos(window w) {
		auto win = get_window_data(w);

		POINT pos;
		if (GetCursorPos(&pos)) {
			ScreenToClient(win->hWnd, &pos);
			return { pos.x, pos.y };
		}
		assert(false);
		return {};
	}

	void set_cursor_pos(window w, int2 pos) {
		if (pos == get_cursor_pos(w)) return;

		auto win = get_window_data(w);

		win->LastCursorPos = pos;

		POINT point = { pos.x, pos.y };
		ClientToScreen(win->hWnd, &point);
		SetCursorPos(point.x, point.y);
	}

	int2 get_pos(window w) {
		auto win = get_window_data(w);

		POINT pos = { 0, 0 };
		ClientToScreen(win->hWnd, &pos);
		return { pos.x, pos.y };
	}

	void set_pos(window w, int2 pos) {
		if (pos == get_pos(w)) return;

		auto win = get_window_data(w);

		RECT rect = { pos.x, pos.y, pos.x, pos.y };

		if (IS_WINDOWS_10_ANNIVERSARY_UPDATE_OR_GREATER()) {
			AdjustWindowRectExForDpi(&rect, get_window_style(win), false, get_window_ex_style(win), GetDpiForWindow(win->hWnd));
		}
		else {
			AdjustWindowRectEx(&rect, get_window_style(win), false, get_window_ex_style(win));
		}
		SetWindowPos(win->hWnd, null, rect.left, rect.top, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
	}

	int2 get_size(window w) {
		auto win = get_window_data(w);

		RECT rect;
		GetClientRect(win->hWnd, &rect);
		return { rect.right, rect.bottom };
	}

	void set_size(window w, int2 size) {
		auto win = get_window_data(w);

		win->DisplayMode.Width = size.x;
		win->DisplayMode.Height = size.y;

		if (size == get_size(w)) return;

		if (win->Monitor) {
			if (win->Monitor->Window == w) {
				acquire_monitor(win);
				fit_to_monitor(win);
			}
		}
		else {
			RECT rect = { 0, 0, size.x, size.y };

			if (IS_WINDOWS_10_ANNIVERSARY_UPDATE_OR_GREATER()) {
				AdjustWindowRectExForDpi(&rect, get_window_style(win), false, get_window_ex_style(win), GetDpiForWindow(win->hWnd));
			}
			else {
				AdjustWindowRectEx(&rect, get_window_style(win), false, get_window_ex_style(win));
			}

			SetWindowPos(win->hWnd, HWND_TOP, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOZORDER);
		}
	}

	int2 get_framebuffer_size(window w) { return get_size(w); }

	rect get_adjusted_bounds(window w) {
		auto win = get_window_data(w);

		int2 size = get_size(w);

		RECT rect;
		SetRect(&rect, 0, 0, size.x, size.y);

		if (IS_WINDOWS_10_ANNIVERSARY_UPDATE_OR_GREATER()) {
			AdjustWindowRectExForDpi(&rect, get_window_style(win), false, get_window_ex_style(win), GetDpiForWindow(win->hWnd));
		}
		else {
			AdjustWindowRectEx(&rect, get_window_style(win), false, get_window_ex_style(win));
		}

		::rect r;
		r.Top = -rect.top;
		r.Left = -rect.left;
		r.Bottom = rect.bottom;
		r.Right = rect.right;
		return r;
	}

	void set_size_limits(window w, int2 minDimension, int2 maxDimension) {
		auto win = get_window_data(w);

		if (minDimension.x != DONT_CARE && minDimension.y != DONT_CARE) {
			if (minDimension.x < 0 || minDimension.y < 0) {
				print(">>> {}:{} Invalid window minimum size ({}x{}).\n", __FILE__, __LINE__, minDimension.x, minDimension.y);
				return;
			}
		}

		if (maxDimension.x != DONT_CARE && maxDimension.y != DONT_CARE) {
			if (maxDimension.x < 0 || maxDimension.y < 0 || maxDimension.x < minDimension.x ||
				maxDimension.y < minDimension.y) {
				print(">>> {}:{} Invalid window maximum size ({}x{}).\n", __FILE__, __LINE__, maxDimension.x, maxDimension.y);
				return;
			}
		}

		win->MinW = minDimension.x;
		win->MinH = minDimension.y;
		win->MaxW = maxDimension.x;
		win->MaxH = maxDimension.y;

		if (win->Monitor || !(win->Flags & RESIZABLE)) return;

		RECT area;
		GetWindowRect(win->hWnd, &area);
		MoveWindow(win->hWnd, area.left, area.top, area.right - area.left, area.bottom - area.top, true);
	}


	cursor* make_cursor(bitmap* image, int2 hotSpot) {
		auto* c = malloc<cursor>();

		c->PlatformData.Win32.hCursor = (HCURSOR)create_icon(image, hotSpot.x, hotSpot.y, false);
		if (!c->PlatformData.Win32.hCursor) {
			print(">>> {}:{} Failed to create os cursor.\n", __FILE__, __LINE__);
			return null;
		}

		c->PlatformData.Win32.ShouldDestroy = true;

		c->Next = CursorsList;
		CursorsList = c;

		return c;
	}

	cursor* make_cursor(cursor_type osCursor) {
		const wchar* id = null;
		if (osCursor == CURSOR_APPSTARTING) id = IDC_APPSTARTING;
		if (osCursor == CURSOR_ARROW) id = IDC_ARROW;
		if (osCursor == CURSOR_IBEAM) id = IDC_IBEAM;
		if (osCursor == CURSOR_CROSSHAIR) id = IDC_CROSS;
		if (osCursor == CURSOR_HAND) id = IDC_HAND;
		if (osCursor == CURSOR_HELP) id = IDC_HELP;
		if (osCursor == CURSOR_NO) id = IDC_NO;
		if (osCursor == CURSOR_RESIZE_ALL) id = IDC_SIZEALL;
		if (osCursor == CURSOR_RESIZE_NESW) id = IDC_SIZENESW;
		if (osCursor == CURSOR_RESIZE_NS) id = IDC_SIZENS;
		if (osCursor == CURSOR_RESIZE_NWSE) id = IDC_SIZENWSE;
		if (osCursor == CURSOR_RESIZE_WE) id = IDC_SIZEWE;
		if (osCursor == CURSOR_UP_ARROW) id = IDC_UPARROW;
		if (osCursor == CURSOR_WAIT) id = IDC_WAIT;
		assert(id);

		auto* c = malloc<cursor>();

		c->PlatformData.Win32.hCursor = (HICON)LoadImageW(null, id, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
		if (!c->PlatformData.Win32.hCursor) {
			print(">>> {}:{} Failed to create os cursor.\n", __FILE__, __LINE__);
			return null;
		}

		c->PlatformData.Win32.ShouldDestroy = false;

		c->Next = CursorsList;
		CursorsList = c;

		return c;
	}

	void free_cursor(cursor* c) {
		if (c->PlatformData.Win32.ShouldDestroy) DestroyCursor(c->PlatformData.Win32.hCursor);

		cursor** prev = &CursorsList;
		while (*prev != c) prev = &((*prev)->Next);
		*prev = c->Next;

		free(c);
	}
}

#endif  // OS == WINDOWS
