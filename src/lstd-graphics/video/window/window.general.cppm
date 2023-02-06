export module g.video.window.general;

export import g.video.window.event;
export import g.bitmap;

export {
	struct monitor;

	enum cursor_type : u32 {
		CURSOR_APPSTARTING,  // Standard arrow and small hourglass

		CURSOR_ARROW,
		CURSOR_IBEAM,
		CURSOR_CROSSHAIR,
		CURSOR_HAND,

		CURSOR_HELP,  // Arrow and question mark
		CURSOR_NO,    // Slashed circle

		CURSOR_RESIZE_ALL,
		CURSOR_RESIZE_NESW,
		CURSOR_RESIZE_NS,
		CURSOR_RESIZE_NWSE,
		CURSOR_RESIZE_WE,

		CURSOR_UP_ARROW,  // Vertical arrow
		CURSOR_WAIT       // Hourglass
	};

	struct cursor {
		union platform_data {
			struct {
				void* hCursor = null;
				bool ShouldDestroy = false;
			} Win32;
		} PlatformData{};

		// We keep track of created cursors in a linked list
		cursor* Next = null;
	};

	cursor* make_cursor(bitmap* image, int2 hotSpot);
	cursor* make_cursor(cursor_type osCursor);

	void free_cursor(cursor* c);

	//
	// @ThreadSafety.
	// Right now, functions dealing with windows must be called from the main thread only.
	//

	// Used to indicate that you don't care about the starting position of the window.
	// Also used in the forced aspect ratio.
	// .. also used in forced min/max dimensions.
	constexpr auto DONT_CARE = 0x1FFF0000;

	// Used to indicate that the window should be centered on the screen
	constexpr auto CENTERED = 0x2FFF0000;

	// Returns a handle which can be safely passed around and copied.
	// If the routine fails, the returned handle has value == window::INVALID_ID.
	//
	// You can check this with an if:
	//    e.g.     if (create_window(...)) {}
	//
	// You can use _DONT_CARE_ or _CENTERED_ for _x_ and _y_ if
	// you don't want to specify them explicitly.
	window create_window(string title, s32 x, s32 y, s32 width, s32 height, u32 flags);

	// Call in your main program loop. Handles messages for all windows.
	void update_windows();

	// Destroys the window
	void free_window(window win);

	enum window_flags : u32 {
		SHOWN = BIT(2),  // Window is visible
		HIDDEN = BIT(3),  // Window is not visible
		MINIMIZED = BIT(4),  // Window is minimized
		MAXIMIZED = BIT(5),  // Window is maximized
		FOCUSED = BIT(6),  // Window is focused

		BORDERLESS = BIT(7),  // No window decoration; Specify in _init()_ or use _set_borderless()_.
		RESIZABLE = BIT(8),  // Window can be resized; Specify in _init()_ or use _set_resizable()_.

		AUTO_MINIMIZE = BIT(9),  // Indicates whether a full screen window is minimized on focus loss
		// Specify in _init()_ or use _set_auto_minimize()_.

		ALWAYS_ON_TOP = BIT(10),  // Also called floating, topmost, etc.
		// Specify in _init()_ or use _set_always_on_top()_.

		FOCUS_ON_SHOW = BIT(11),  // Specify in _init()_ or use _set_focus_on_show()_.

		ALPHA = BIT(12),  // Can only be specified when creating the window! Windows without this flag
		// don't support _get_opacity()_ and _set_opacity()_.

		VSYNC = BIT(13),  // Specify in _init()_ or use _set_vsync()_.
		CLOSE_ON_ALT_F4 = BIT(14),  // Specify in _init()_ or use _set_close_on_alt_f4()_.

		// Specifies that the window is transparent for mouse input (e.g. when testing if
		// a window behind this one is hovered, but this window has MOUSE_PASS_THROUGH, it treats it
		// as "transparent" and continues to test on the window behind it). Specify in _init()_ or
		// manually modify the flags. Note that this flag is unrelated to visual transparency.
		// Only valid on undecorated windows.
		// Specify in _init()_ or use _set_mouse_pass_through()_.
		MOUSE_PASS_THROUGH = BIT(15),

		// Specifies whether the window content area should be
		// resized based on the content scale of any monitor it is placed on.
		// This includes the initial placement when the window is created.
		SCALE_TO_MONITOR = BIT(16),

		// When initializing the window we only pay attention to these flags:
		CREATION_FLAGS = SHOWN | BORDERLESS | RESIZABLE | AUTO_MINIMIZE | ALWAYS_ON_TOP | FOCUS_ON_SHOW | ALPHA |
		VSYNC | CLOSE_ON_ALT_F4 | MOUSE_PASS_THROUGH | SCALE_TO_MONITOR
	};

	enum cursor_mode : u8 {
		CURSOR_NORMAL = 0,  // Makes the cursor visible and behaving normally.

		CURSOR_HIDDEN,  // Makes the cursor invisible when it is over the content area of the window but does not
		// restrict the cursor from leaving.

		CURSOR_DISABLED  // Hides and grabs the cursor, providing virtual and unlimited cursor movement.
	};

	// Use this to get all current flags, see the enum above.
	u32 get_flags(window win);

	// Use these routines to register callbacks in order to be notified about events.
	// See event.h
	s64 connect_event(window win, delegate<bool(event)> sb);
	bool disconnect_event(window win, s64 cb);

	// True if the window is about to get ended.
	bool is_destroying(window win);

	[[nodiscard("Leak")]] string get_title(window win);  // The caller is responsible for freeing
	void set_title(window win, string title);

	void set_fullscreen(window win, monitor* mon, s32 width, s32 height, s32 refreshRate = DONT_CARE);
	bool is_fullscreen(window win);

	// The image data should be 32-bit, little-endian, non-premultiplied RGBA.
	// The pixels should be arranged canonically as sequential rows, starting from the top-left corner.
	//
	// We choose the icons with the closest sizes which we need.
	void set_icon(window win, array<bitmap> icons);

	void set_cursor(window win, cursor* curs);

	int2 get_cursor_pos(window win);
	void set_cursor_pos(window win, int2 pos);
	inline void set_cursor_pos(window win, s32 x, s32 y) { set_cursor_pos(win, { x, y }); }

	int2 get_pos(window win);
	void set_pos(window win, int2 pos);
	inline void set_pos(window win, s32 x, s32 y) { set_pos(win, { x, y }); }

	int2 get_size(window win);
	void set_size(window win, int2 size);
	inline void set_size(window win, s32 width, s32 height) { set_size(win, { width, height }); }

	// May not map 1:1 with window size (e.g. Retina display on Mac)
	int2 get_framebuffer_size(window win);

	// Gets the full area the window occupies (including title bar, borders, etc.),
	// relative to the window's position. So to get the absolute top-left corner
	// use: windowPos.x + get_adjusted_bounds().left;
	rect get_adjusted_bounds(window win);

	// You can call these with _DONT_CARE_ to reset.
	void set_size_limits(window win, int2 minDimension, int2 maxDimension);
	inline void set_size_limits(window win, s32 minWidth, s32 minHeight, s32 maxWidth, s32 maxHeight) {
		set_size_limits(win, { minWidth, minHeight }, { maxWidth, maxHeight });
	}

	// You can call this with DONT_CARE to reset.
	void set_forced_aspect_ratio(window win, s32 numerator, s32 denominator);

	void set_raw_mouse(window win, bool enabled);
	void set_cursor_mode(window win, cursor_mode mode);
	cursor_mode get_cursor_mode(window win);

	f32 get_opacity(window win); // 0.0f - 1.0f
	void set_opacity(window win, f32 opacity); // 0.0f - 1.0f

	void set_borderless(window win, bool enabled);
	void set_resizable(window win, bool enabled);
	void set_always_on_top(window win, bool enabled);
	void set_auto_minimize(window win, bool enabled);
	void set_focus_on_show(window win, bool enabled);
	void set_vsync(window win, bool enabled);
	void set_close_on_alt_f4(window win, bool enabled);
	void set_mouse_pass_through(window win, bool enabled);

	bool is_hovered(window win);
	bool is_visible(window win);

	void show(window win);
	void hide(window win);
	void minimize(window win);
	void restore(window win);
	void maximize(window win);
	void focus(window win);

	void request_attention(window win); // Flashes the window
}