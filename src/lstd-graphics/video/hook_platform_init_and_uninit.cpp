import g.video.window;
import g.video.monitor;

typedef void(__cdecl* _PVFV)(void);

#pragma section("LSTD_THINGS_THAT_SHOULD_RUN_BEFORE_MAIN$b", read)
#pragma section("LSTD_THINGS_THAT_SHOULD_RUN_BEFORE_MAIN$c", read)
#pragma section("LSTD_THINGS_THAT_SHOULD_RUN_AFTER_MAIN$b", read)
#pragma section("LSTD_THINGS_THAT_SHOULD_RUN_AFTER_MAIN$c", read)

#define ADD_EARLY_INITIALIZER(fn) __declspec(allocate("LSTD_THINGS_THAT_SHOULD_RUN_BEFORE_MAIN$b")) \
    const _PVFV initializer##fn = fn
#define ADD_INITIALIZER(fn)       __declspec(allocate("LSTD_THINGS_THAT_SHOULD_RUN_BEFORE_MAIN$c")) \
    const _PVFV initializer##fn = fn

#define ADD_EARLY_UNINITIALIZER(fn) __declspec(allocate("LSTD_THINGS_THAT_SHOULD_RUN_AFTER_MAIN$b")) \
    const _PVFV uninitializer##fn = fn
#define ADD_UNINITIALIZER(fn)       __declspec(allocate("LSTD_THINGS_THAT_SHOULD_RUN_AFTER_MAIN$c")) \
    const _PVFV uninitializer##fn = fn

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

ADD_EARLY_INITIALIZER(platform_hook_monitor_init);
ADD_INITIALIZER(platform_hook_window_init);

ADD_EARLY_UNINITIALIZER(platform_hook_window_uninit);
ADD_UNINITIALIZER(platform_hook_monitor_uninit);
