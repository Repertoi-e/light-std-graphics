import g.video.window;
import g.video.monitor;

extern "C" {
	void platform_hook_monitor_init() {
		platform_monitor_init();
	}

	void platform_hook_window_init() {
		platform_window_init();
	}

	void platform_hook_window_uninit() {
		platform_window_uninit();
	}

	void platform_hook_monitor_uninit() {
		platform_monitor_uninit();
	}
}

#pragma section("LSTD_B$b", read)
#pragma section("LSTD_B$c", read)
#pragma section("LSTD_A$b", read)
#pragma section("LSTD_A$c", read)

LSTD_ADD_EARLY_INITIALIZER(platform_hook_monitor_init);
LSTD_ADD_INITIALIZER(platform_hook_window_init);

LSTD_ADD_EARLY_UNINITIALIZER(platform_hook_window_uninit);
LSTD_ADD_UNINITIALIZER(platform_hook_monitor_uninit);

#pragma comment(linker, "/SECTION:LSTD_B$b,s")

