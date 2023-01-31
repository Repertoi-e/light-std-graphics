export module g.video.window.event;

export import "lstd.h";
export import g.math;

#if OS == WINDOWS
// This exports two tables:
// u32 g_KeycodeHidToNative[256];
// u32 g_KeycodeNativeToHid[256];
// which are used to map from platform w/l-param-scan-things-codes-whatever to HID keys.
//
export import window.win32.keycode.cppm;
#else
#error Implement.
#endif

export {
	// Unique handle, we book keep data in our own separate way (see window.win32.cppm) 
	// on Windows this is the HWND as well. null if invalid.
	using window = void*;

	enum : u32 {
		Mouse_Button_Left = 0,
		Mouse_Button_Right = 1,
		Mouse_Button_Middle = 2,
		Mouse_Button_X1 = 3,
		Mouse_Button_X2 = 4,

		Mouse_Button_Last = Mouse_Button_X2
	};

	// Convert a mouse button from it's name to code
	u32 mouse_button_code_from_name(string name) {
		if (strings_match(name, "Left")) {
			return Mouse_Button_Left;
		}
		else if (strings_match(name, "Right")) {
			return Mouse_Button_Right;
		}
		else if (strings_match(name, "Middle")) {
			return Mouse_Button_Middle;
		}
		else if (strings_match(name, "X1")) {
			return Mouse_Button_X1;
		}
		else if (strings_match(name, "X2")) {
			return Mouse_Button_X2;
		}
		assert(false);
		return 0;
	}

	// Convert a mouse button from it's code to name
	string mouse_button_name_from_code(u32 code) {
		if (code == Mouse_Button_Left) {
			return "Left";
		}
		else if (code == Mouse_Button_Right) {
			return "Right";
		}
		else if (code == Mouse_Button_Middle) {
			return "Middle";
		}
		else if (code == Mouse_Button_X1) {
			return "X1";
		}
		else if (code == Mouse_Button_X2) {
			return "X2";
		}
		assert(false);
		return "";
	}

	// Key codes in the engine correspond to the USB HID codes.
	// Each constant in this file defines the location of a key and not its
	// character meaning. For example, Key_A corresponds to A on a US
	// keyboard, but it corresponds to Q on a French keyboard layout.
	enum : u32 {
		/* Zero, does not correspond to any key. */
		Key_None = 0,

		/* Keycode definitions. */
		Key_A = 4,
		Key_B = 5,
		Key_C = 6,
		Key_D = 7,
		Key_E = 8,
		Key_F = 9,
		Key_G = 10,
		Key_H = 11,
		Key_I = 12,
		Key_J = 13,
		Key_K = 14,
		Key_L = 15,
		Key_M = 16,
		Key_N = 17,
		Key_O = 18,
		Key_P = 19,
		Key_Q = 20,
		Key_R = 21,
		Key_S = 22,
		Key_T = 23,
		Key_U = 24,
		Key_V = 25,
		Key_W = 26,
		Key_X = 27,
		Key_Y = 28,
		Key_Z = 29,
		Key_1 = 30,
		Key_2 = 31,
		Key_3 = 32,
		Key_4 = 33,
		Key_5 = 34,
		Key_6 = 35,
		Key_7 = 36,
		Key_8 = 37,
		Key_9 = 38,
		Key_0 = 39,
		Key_Escape = 41,
		Key_Delete = 42,
		Key_Tab = 43,
		Key_Space = 44,
		Key_Minus = 45,
		Key_Equals = 46,
		Key_LeftBracket = 47,
		Key_RightBracket = 48,
		Key_Backslash = 49,
		Key_Semicolon = 51,
		Key_Quote = 52,
		Key_Grave = 53,
		Key_Comma = 54,
		Key_Period = 55,
		Key_Slash = 56,
		Key_CapsLock = 57,
		Key_F1 = 58,
		Key_F2 = 59,
		Key_F3 = 60,
		Key_F4 = 61,
		Key_F5 = 62,
		Key_F6 = 63,
		Key_F7 = 64,
		Key_F8 = 65,
		Key_F9 = 66,
		Key_F10 = 67,
		Key_F11 = 68,
		Key_F12 = 69,
		Key_PrintScreen = 70,
		Key_ScrollLock = 71,
		Key_Pause = 72,
		Key_Insert = 73,
		Key_Home = 74,
		Key_PageUp = 75,
		Key_DeleteForward = 76,
		Key_End = 77,
		Key_PageDown = 78,
		Key_Right = 79,
		Key_Left = 80,
		Key_Down = 81,
		Key_Up = 82,
		KP_NumLock = 83,
		KP_Divide = 84,
		KP_Multiply = 85,
		KP_Subtract = 86,
		KP_Add = 87,
		KP_Enter = 88,
		KP_1 = 89,
		KP_2 = 90,
		KP_3 = 91,
		KP_4 = 92,
		KP_5 = 93,
		KP_6 = 94,
		KP_7 = 95,
		KP_8 = 96,
		KP_9 = 97,
		KP_0 = 98,
		KP_Point = 99,
		Key_NonUSBackslash = 100,
		KP_Equals = 103,
		Key_F13 = 104,
		Key_F14 = 105,
		Key_F15 = 106,
		Key_F16 = 107,
		Key_F17 = 108,
		Key_F18 = 109,
		Key_F19 = 110,
		Key_F20 = 111,
		Key_F21 = 112,
		Key_F22 = 113,
		Key_F23 = 114,
		Key_F24 = 115,
		Key_Help = 117,
		Key_Menu = 118,
		Key_Mute = 127,
		Key_SysReq = 154,
		Key_Return = 158,
		KP_Clear = 216,
		KP_Decimal = 220,
		Key_LeftControl = 224,
		Key_LeftShift = 225,
		Key_LeftAlt = 226,
		Key_LeftGUI = 227,
		Key_RightControl = 228,
		Key_RightShift = 229,
		Key_RightAlt = 230,
		Key_RightGUI = 231,

		Key_Last = Key_RightGUI
	};
}

// Copyright 2019 Dietrich Epp.
// The following part of the code which converts HID values to 
// strings is licensed under the terms of the MIT license.
// https://github.com/depp/keycode

// These following tables have been automatically generated:
static const char* KEY_ID_DATA =
"\0A\0B\0C\0CapsLock\0Comma\0D\0Delete\0DeleteForward\0E\0End\0Escape\0F\0"
"F1\0F10\0F11\0F12\0F13\0F14\0F15\0F16\0F17\0F18\0F19\0F2\0F20\0F21\0F22\0"
"F23\0F24\0F3\0F4\0F5\0F6\0F7\0F8\0F9\0G\0Grave\0H\0Help\0Home\0Insert\0J"
"\0K\0KP0\0KP1\0KP2\0KP3\0KP4\0KP5\0KP6\0KP7\0KP8\0KP9\0KPAdd\0KPClear\0KP"
"Decimal\0KPDivide\0KPEnter\0KPEquals\0KPMultiply\0KPNumLock\0KPPoint\0KPS"
"ubtract\0L\0Left\0LeftAlt\0LeftBracket\0LeftControl\0LeftGUI\0LeftShift\0"
"M\0Menu\0Minus\0Mute\0N\0NonUSBackslash\0O\0P\0PageDown\0PageUp\0Pause\0P"
"eriod\0PrintScreen\0Q\0Quote\0R\0Return\0Right\0RightAlt\0RightBracket\0R"
"ightControl\0RightGUI\0RightShift\0S\0ScrollLock\0Semicolon\0Slash\0Space"
"\0SysReq\0T\0Tab\0U\0V\0W\0X\0Y\0Z";

static s16 KEY_ID_OFF[] = {
	0, 0, 0, 0, 1, 3, 5, 22, 45, 58, 147, 155, 353, 174, 176, 308, 365, 383, 400, 402, 445, 453,
	523, 565, 571, 573, 575, 577, 579, 581, 61, 73, 77, 81, 85, 89, 93, 97, 101, 65, 0, 51, 24, 567,
	552, 372, 261, 323, 477, 390, 0, 536, 447, 149, 16, 426, 546, 7, 60, 103, 126, 129, 132, 135, 138, 141,
	144, 63, 67, 71, 433, 525, 420, 167, 162, 413, 31, 47, 404, 462, 310, 408, 417, 279, 242, 268, 297, 218,
	251, 182, 186, 190, 194, 198, 202, 206, 210, 214, 178, 289, 385, 0, 0, 259, 75, 79, 83, 87, 91, 95,
	99, 106, 110, 114, 118, 122, 0, 157, 367, 0, 0, 0, 0, 0, 0, 0, 0, 378, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	558, 0, 0, 0, 455, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 224, 0, 0, 0,
	232, 0, 0, 0, 335, 355, 315, 347, 490, 512, 468, 503 };

static s32 KEY_ID_ORDER[] = { 39, 30, 31, 32, 33, 34, 35, 36, 37, 38, 4, 5, 49, 6, 57, 54, 7, 42,
							 76, 81, 8, 77, 46, 41, 9, 58, 67, 68, 69, 104, 105, 106, 107, 108, 109, 110,
							 59, 111, 112, 113, 114, 115, 60, 61, 62, 63, 64, 65, 66, 10, 53, 11, 117, 74,
							 12, 73, 13, 14, 98, 89, 90, 91, 92, 93, 94, 95, 96, 97, 87, 216, 220, 84,
							 88, 103, 85, 83, 99, 86, 15, 80, 226, 47, 224, 227, 225, 16, 118, 45, 127, 17,
							 100, 18, 19, 78, 75, 72, 55, 70, 20, 52, 21, 158, 79, 230, 48, 228, 231, 229,
							 22, 71, 51, 56, 44, 154, 23, 43, 24, 82, 25, 26, 27, 28, 29 };

export {
	u32 key_code_from_name(string name) {
		char buffer[14];

		s32 n;
		for (n = 0; n < 14 && length(name) != n; ++n) {
			auto c = (char)name[n];
			if ('A' <= c && c <= 'Z') c |= 32;
			buffer[n] = c;
		}
		if (length(name) != n) return 0;

		// Binary search in sorted list of identifiers
		s32 left = 0, right = sizeof(KEY_ID_ORDER) / sizeof(s32), center;
		while (left < right) {
			center = left + (right - left) / 2;
			u32 code = KEY_ID_ORDER[center];
			auto* p = KEY_ID_DATA + KEY_ID_OFF[code];
			for (int i = 0;; i++) {
				if (i >= n) {
					if (p[i] != '\0') {
						right = center;
						break;
					}
					return code;
				}
				s32 d = buffer[i] - ((char)p[i] | 32);
				if (d < 0) {
					right = center;
					break;
				}
				else if (d > 0) {
					left = center + 1;
					break;
				}
			}
		}
		return 0;
	}

	string key_name_from_code(u32 code) {
		if (code >= 232) return "";
		s16 off = KEY_ID_OFF[code];
		if (off == 0) return "";
		return string(KEY_ID_DATA + off);
	}
}

export {
	enum : u32 {
		Modifier_Shift = BIT(0),
		Modifier_Control = BIT(1),
		Modifier_Alt = BIT(2),
		Modifier_Super = BIT(3),
		Modifier_CapsLock = BIT(4),
		Modifier_NumLock = BIT(5)
	};

	// @TODO: Specialize this for fmt::formatter
	struct event {
		enum type : s32 {
			//
			// Mouse events:
			//
			Mouse_Button_Pressed = 0,  // Sets _Button_ and _DoubleClicked_
			Mouse_Button_Released = 1,  // Sets _Button_
			Mouse_Wheel_Scrolled = 2,  // Sets _ScrollX_ and _ScrollY_
			Mouse_Moved = 3,  // Sets _X_, _Y, _DX_ and _DY_
			Mouse_Entered_Window = 4,  // Doesn't set anything
			Mouse_Left_Window = 5,  // Doesn't set anything

			//
			// Keyboard events:
			//
			Key_Pressed = 6,  // Sets _KeyCode_
			Key_Released = 7,  // Sets _KeyCode_
			Key_Repeated = 8,  // Sets _KeyCode_

			Code_Point_Typed = 9,  // Sets _CP_ to the UTF-32 encoded code point which was typed

			//
			// Window events:
			//
			Window_Closed = 10,  // Doesn't set anything
			Window_Minimized = 11,  // Sets _Minimized_ (true if the window was minimized, false if restored)
			Window_Maximized = 12,  // Sets _Maximized_ (true if the window was maximized, false if restored)
			Window_Focused = 13,  // Sets _Focused_ (true if the window gained focus, false if lost)

			Window_Moved = 14,  // Sets _X_ and _Y_
			Window_Resized = 15,  // Sets _Width_ and _Height_

			// May not map 1:1 with Window_Resized (e.g. Retina display on Mac)
			Window_Framebuffer_Resized = 16,  // Sets _Width_ and _Height_

			Window_Refreshed = 17,  // Doesn't set anything
			Window_Content_Scale_Changed = 18,  // Sets _Scale_
			Window_Files_Dropped = 19,  // Sets _Paths_ (the array contains a list of all files dropped)

			// Use this for any platform specific behaviour that can't be handled just using this library
			// This event is sent for every single platform message (including the ones handled by the events above!)
			Window_Platform_Message_Sent = 20  // Sets _Message_, _Param1_, _Param2_
		};

		window Window;          // This is always set to the window which the event originated from
		type Type = (type)-1;  // This is always set to the type of event (one of the above)

		union {
			u32 Button;   // Only set on mouse button pressed/released
			u32 KeyCode;  // Only set on keyboard pressed/released/repeated events
		};

		float2 Scale;  // Only set on Window_Content_Scale_Changed

		union {
			bool DoubleClicked;  // Only set on Mouse_Button_Pressed
			struct {
				s32 ScrollX, ScrollY;  // Only set on Mouse_Wheel_Scrolled
			};

			struct {
				u32 X, Y;    // Set on Mouse_Moved and Window_Moved
				s32 DX, DY;  // Only set on Mouse_Moved
			};

			code_point CP;  // Only set on Code_Point_Typed

			struct {
				u32 Width, Height;  // Only set on Window_Resized
			};

			bool Minimized;  // Only set on Window_Minimized
			bool Maximized;  // Only set on Window_Maximized
			bool Focused;    // Only set on Window_Focused

			struct {
				u32 Message;  // Only set on Window_Platform_Message_Sent
				u64 Param1;   // Only set on Window_Platform_Message_Sent
				s64 Param2;   // Only set on Window_Platform_Message_Sent
			};
		};

		// Gets temporarily allocated in the window procedure, the event doesn't own this.
		array<string> Paths;  // Only set on Window_Files_Dropped
	};
}