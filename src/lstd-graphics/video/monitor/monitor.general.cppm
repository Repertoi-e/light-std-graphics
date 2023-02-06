export module g.video.monitor.general;

export import g.math;

import "lstd.h";
import lstd.delegate;

export {
	struct display_mode {
		// Use this on RGB bits or refresh rate when setting the display mode for a monitor
		static constexpr s32 DONT_CARE = -1;

		s32 Width = 0, Height = 0;
		s32 RedBits = 0, GreenBits = 0, BlueBits = 0;
		s32 RefreshRate = 0;

		auto operator<=>(display_mode no_copy other) const {
			s32 bpp = RedBits + GreenBits + BlueBits;
			s32 otherBPP = other.RedBits + other.GreenBits + other.BlueBits;

			// First sort on color bits per pixel
			if (bpp != otherBPP) return bpp <=> otherBPP;

			s32 area = Width * Height;
			s32 otherArea = other.Width * other.Height;

			// Then sort on screen area
			if (area != otherArea) return area <=> otherArea;

			return RefreshRate <=> other.RefreshRate;
		}

		bool operator==(display_mode no_copy other) const { return (*this <=> other) == 0; }
		bool operator!=(display_mode no_copy other) const { return !(*this == other); }
	};

	// These don't get created or freed in a straight-forward way, but instead callers 
	// should know that they are managed by the platform layer.
	struct monitor {
		union platform_data {
			struct {
				void* hMonitor = null;

				// 32 matches the static size of DISPLAY_DEVICE.DeviceName
				wchar_t AdapterName[32]{}, DisplayName[32]{};
				char PublicAdapterName[32]{}, PublicDisplayName[32]{};

				bool ModesPruned = false, ModeChanged = false;
			} Win32;
		} PlatformData{};

		string Name;

		// Physical dimensions in millimeters
		s32 WidthMM = 0, HeightMM = 0;

		// The handle to the window whose video mode is current on this monitor
		// i.e. fullscreen. By default it's an invalid handle - if no window is fullscreen.
		void* Window;

		array<display_mode> DisplayModes;
		display_mode CurrentMode;
	};

	struct monitor_event {
		enum action {
			CONNECTED,
			DISCONNECTED
		};

		monitor* Monitor = null;
		action Action;
	};

	// For monitor connect/disconnect events
	s64 monitor_connect_callback(delegate<void(monitor_event)> cb);
	bool monitor_disconnect_callback(s64 cb);

	display_mode monitor_get_current_display_mode(monitor* mon);

	// Work area is the screen excluding taskbar and other docked bars
	rect get_work_area(monitor* mon);

	bool set_display_mode(monitor* mon, display_mode desired);
	void restore_display_mode(monitor* mon);

	int2 get_pos(monitor* mon);
	float2 get_content_scale(monitor* mon);

	// You usually don't need to do this, but if for some reason _get_monitors()_ returns an empty array, call this.
	// Note: We register windows to receive notifications on device connected/disconnected, which means this will get
	// called automatically if your window is already running.
	// The problem described above might happen when initializing code.
	// @Cleanup: Figure this out!
	void poll_monitors();

	// Returns a pointer to the monitor which contains the window _win_.
	//
	// Don't free the result of this function. This library follows the convention that if the function is not marked as [[nodiscard]], the returned value should not be freed.
	monitor* monitor_from_window(void* win);

	// Returns an array of all available monitors connected to the computer.
	//
	// Don't free the result of this function. This library follows the convention that if the function is not marked as [[nodiscard]], the returned value should not be freed.
	array<monitor*> get_monitors();

	// Returns get_monitors()[0]
	//
	// Don't free the result of this function. This library follows the convention that if the function is not marked as [[nodiscard]], the returned value should not be freed.
	monitor* get_primary_monitor();
}
