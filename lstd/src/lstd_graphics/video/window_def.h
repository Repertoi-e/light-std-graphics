#pragma once

#include "../memory/pixel_buffer.h"
#include "cursor.h"
#include "lstd/math.h"
#include "lstd/memory/signal.h"

LSTD_BEGIN_NAMESPACE

//
// @ThreadSafety.
// Right now, functions dealing with windows must be called from the main thread only.
//

struct event;
struct monitor;

struct window {
    // Used to indicate that you don't care about the starting position of the window.
    // Also used in the forced aspect ratio.
    // .. also used in forced min/max dimensions.
    static constexpr auto DONT_CARE = 0x1FFF0000;

    // Used to indicate that the window should be centered on the screen
    static constexpr auto CENTERED = 0x2FFF0000;

    enum flags : u32 {
        SHOWN     = BIT(2),  // Window is visible
        HIDDEN    = BIT(3),  // Window is not visible
        MINIMIZED = BIT(4),  // Window is minimized
        MAXIMIZED = BIT(5),  // Window is maximized
        FOCUSED   = BIT(6),  // Window is focused

        BORDERLESS = BIT(7),  // No window decoration; Specify in _init()_ or use _set_borderless()_.
        RESIZABLE  = BIT(8),  // Window can be resized; Specify in _init()_ or use _set_resizable()_.

        AUTO_MINIMIZE = BIT(9),  // Indicates whether a full screen window is minimized on focus loss
                                 // Specify in _init()_ or use _set_auto_minimize()_.

        ALWAYS_ON_TOP = BIT(10),  // Also called floating, topmost, etc.
                                  // Specify in _init()_ or use _set_always_on_top()_.

        FOCUS_ON_SHOW = BIT(11),  // Specify in _init()_ or use _set_focus_on_show()_.

        ALPHA = BIT(12),  // Can only be specified when creating the window! Windows without this flag
                          // don't support _get_opacity()_ and _set_opacity()_.

        VSYNC           = BIT(13),  // Specify in _init()_ or use _set_vsync()_.
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

    using event_signal_t = signal<bool(const event &), collector_while0<bool>>;

    // _ID_ is set to this when the window is not initialized/destroyed and no longer valid.
    static constexpr u32 INVALID_ID = (u32) -1;

    // Unique ID for each live window.
    // This is the only non-static member in this structure which means that
    // passing around _window_ is cheap and safe to copy. It's meant to be used as a handle.
    // _ID_ is set to the platform specific handle (e.g. HWND for Windows).
    u64 ID = INVALID_ID;

    window() {}

    // Use these routines to register callbacks in order to be notified about events.
    // See event.h
    s64 connect_event(const event_signal_t::callback_t &sb);
    bool disconnect_event(s64 cb);

    // Use this to get all current flags and set options, see the enum _flags_ above.
    u32 get_flags() const;

    // True if the window is about to get ended.
    bool is_destroying() const;

    [[nodiscard("Leak")]] string get_title();  // The caller is responsible for freeing
    void set_title(string title);

    void set_fullscreen(monitor *mon, s32 width, s32 height, s32 refreshRate = DONT_CARE);
    bool is_fullscreen() const;

    // The image data should be 32-bit, little-endian, non-premultiplied RGBA.
    // The pixels should be arranged canonically as sequential rows, starting from the top-left corner.
    //
    // We choose the icons with the closest sizes which we need.
    void set_icon(array<pixel_buffer> icons);

    void set_cursor(cursor *curs);

    vec2<s32> get_cursor_pos() const;
    void set_cursor_pos(vec2<s32> pos);
    void set_cursor_pos(s32 x, s32 y) { set_pos({x, y}); }

    vec2<s32> get_pos() const;
    void set_pos(vec2<s32> pos);
    void set_pos(s32 x, s32 y) { set_pos({x, y}); }

    vec2<s32> get_size() const;
    void set_size(vec2<s32> size);
    void set_size(s32 width, s32 height) { set_size({width, height}); }

    // May not map 1:1 with window size (e.g. Retina display on Mac)
    vec2<s32> get_framebuffer_size() const;

    // Gets the full area the window occupies (including title bar, borders, etc.),
    // relative to the window's position. So to get the absolute top-left corner
    // use: windowPos.x + get_adjusted_bounds().left;
    rect get_adjusted_bounds() const;

    // You can call these with _DONT_CARE_ to reset.
    void set_size_limits(vec2<s32> minDimension, vec2<s32> maxDimension);
    void set_size_limits(s32 minWidth, s32 minHeight, s32 maxWidth, s32 maxHeight) {
        set_size_limits({minWidth, minHeight}, {maxWidth, maxHeight});
    }

    // You can call this with DONT_CARE to reset.
    void set_forced_aspect_ratio(s32 numerator, s32 denominator);

    void set_raw_mouse(bool enabled);
    void set_cursor_mode(cursor_mode mode);
    cursor_mode get_cursor_mode() const;

    // 0.0f - 1.0f
    f32 get_opacity() const;

    // 0.0f - 1.0f
    void set_opacity(f32 opacity);

    void set_borderless(bool enabled);
    void set_resizable(bool enabled);
    void set_always_on_top(bool enabled);
    void set_auto_minimize(bool enabled);
    void set_focus_on_show(bool enabled);
    void set_vsync(bool enabled);
    void set_close_on_alt_f4(bool enabled);
    void set_mouse_pass_through(bool enabled);

    bool is_hovered() const;
    bool is_visible() const;

    void show();
    void hide();
    void minimize();
    void restore();
    void maximize();
    void focus();

    // Flashes the window
    void request_attention();

    bool operator==(window other) const { return ID == other.ID; }
    bool operator!=(window other) const { return ID != other.ID; }

    operator bool() const { return ID != INVALID_ID; }
    operator void *() const { return (void *) ID; }
};

// Returns a handle which can be safely passed around and copied.
// If the routine fails, the returned handle has value == window::INVALID_ID.
//
// You can check this with an if:
//    e.g.     if (os_create_window(...)) {}
//
// You can use _DONT_CARE_ or _CENTERED_ for _x_ and _y_ if
// you don't want to specify them explicitly.
window os_create_window(string title, s32 x, s32 y, s32 width, s32 height, u32 flags);

// Call in your main program loop. Handles messages for all windows.
void os_update_windows();

// Destroys the window
void free(window win);

LSTD_END_NAMESPACE
