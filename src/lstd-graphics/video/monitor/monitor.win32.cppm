module;

#include "lstd/platform/windows.h"

export module g.video.monitor.win32;

export import g.video.monitor.general;

#if OS == WINDOWS
import lstd.os;
import lstd.fmt;
import lstd.signal;

#ifndef DPI_ENUMS_DECLARED
typedef enum {
    PROCESS_DPI_UNAWARE           = 0,
    PROCESS_SYSTEM_DPI_AWARE      = 1,
    PROCESS_PER_MONITOR_DPI_AWARE = 2
} PROCESS_DPI_AWARENESS;
typedef enum {
    MDT_EFFECTIVE_DPI = 0,
    MDT_ANGULAR_DPI   = 1,
    MDT_RAW_DPI       = 2,
    MDT_DEFAULT       = MDT_EFFECTIVE_DPI
} MONITOR_DPI_TYPE;
#endif

#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((HANDLE) -4)
#endif

// shcore.dll function pointer typedefs
typedef HRESULT(WINAPI *PFN_SetProcessDpiAwareness)(PROCESS_DPI_AWARENESS);
typedef HRESULT(WINAPI *PFN_GetDpiForMonitor)(HMONITOR, MONITOR_DPI_TYPE, UINT *, UINT *);
#define SetProcessDpiAwareness shcore.SetProcessDpiAwareness_
#define GetDpiForMonitor shcore.GetDpiForMonitor_

// ntdll.dll function pointer typedefs
typedef LONG(WINAPI *PFN_RtlVerifyVersionInfo)(OSVERSIONINFOEXW *, ULONG, ULONGLONG);
#define RtlVerifyVersionInfo ntdll.RtlVerifyVersionInfo_

struct {
    HINSTANCE hInstance;
    PFN_SetProcessDpiAwareness SetProcessDpiAwareness_;
    PFN_GetDpiForMonitor GetDpiForMonitor_;
} shcore;

struct {
    HINSTANCE hInstance;
    PFN_RtlVerifyVersionInfo RtlVerifyVersionInfo_;
} ntdll;

BOOL is_windows_version_or_greater(WORD major, WORD minor, WORD sp) {
    OSVERSIONINFOEXW osvi = {sizeof(osvi), major, minor, 0, 0, {0}, sp};
    DWORD mask            = VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR;
    ULONGLONG cond        = VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL);
    cond                  = VerSetConditionMask(cond, VER_MINORVERSION, VER_GREATER_EQUAL);
    cond                  = VerSetConditionMask(cond, VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL);
    return RtlVerifyVersionInfo(&osvi, mask, cond) == 0;
}

BOOL is_windows_10_build_or_greater(WORD build) {
    OSVERSIONINFOEXW osvi = {sizeof(osvi), 10, 0, build};
    DWORD mask            = VER_MAJORVERSION | VER_MINORVERSION | VER_BUILDNUMBER;
    ULONGLONG cond        = VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL);
    cond                  = VerSetConditionMask(cond, VER_MINORVERSION, VER_GREATER_EQUAL);
    cond                  = VerSetConditionMask(cond, VER_BUILDNUMBER, VER_GREATER_EQUAL);
    return RtlVerifyVersionInfo(&osvi, mask, cond) == 0;
}

#define IS_WINDOWS_8_OR_GREATER() is_windows_version_or_greater(HIBYTE(_WIN32_WINNT_WIN8), LOBYTE(_WIN32_WINNT_WIN8), 0)
#define IS_WINDOWS_8_POINT_1_OR_GREATER() \
    is_windows_version_or_greater(HIBYTE(_WIN32_WINNT_WINBLUE), LOBYTE(_WIN32_WINNT_WINBLUE), 0)

#define IS_WINDOWS_10_ANNIVERSARY_UPDATE_OR_GREATER() is_windows_10_build_or_greater(14393)
#define IS_WINDOWS_10_CREATORS_UPDATE_OR_GREATER() is_windows_10_build_or_greater(15063)

file_scope array<monitor *> Monitors;
file_scope DWORD ForegroundLockTimeout;
// @ThreadSafety
file_scope signal<void(monitor_event)> MonitorEvent;

void win32_poll_monitors();

void win32_monitor_uninit() {
    free_signal(&MonitorEvent);

    For(Monitors) {
        free(it->Name.Data);
        free(it->DisplayModes.Data);
        free(it);
    }
    free(Monitors.Data);

    SystemParametersInfoW(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, UIntToPtr(ForegroundLockTimeout), SPIF_SENDCHANGE);

    if (shcore.hInstance) FreeLibrary(shcore.hInstance);
    if (ntdll.hInstance) FreeLibrary(ntdll.hInstance);
}

void win32_monitor_init() {
    // To make SetForegroundWindow work as we want, we need to fiddle
    // with the FOREGROUNDLOCKTIMEOUT system setting (we do this as early
    // as possible in the hope of still being the foreground process)
    SystemParametersInfoW(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, &ForegroundLockTimeout, 0);
    SystemParametersInfoW(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, UIntToPtr(0), SPIF_SENDCHANGE);

    shcore.hInstance = LoadLibraryW(L"shcore.dll");
    if (shcore.hInstance) {
        shcore.SetProcessDpiAwareness_ = (PFN_SetProcessDpiAwareness) GetProcAddress(shcore.hInstance, "SetProcessDpiAwareness");
        shcore.GetDpiForMonitor_       = (PFN_GetDpiForMonitor) GetProcAddress(shcore.hInstance, "GetDpiForMonitor");
    }

    ntdll.hInstance = LoadLibraryW(L"ntdll.dll");
    if (ntdll.hInstance) {
        ntdll.RtlVerifyVersionInfo_ = (PFN_RtlVerifyVersionInfo) GetProcAddress(ntdll.hInstance, "RtlVerifyVersionInfo");
    }

    if (IS_WINDOWS_10_CREATORS_UPDATE_OR_GREATER()) {
        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    } else if (IS_WINDOWS_8_POINT_1_OR_GREATER()) {
        SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
    } else {
        SetProcessDPIAware();
    }

    PUSH_ALLOC(platform_get_persistent_allocator()) {
        make_dynamic(&Monitors, 8);
        make_dynamic(&MonitorEvent.Callbacks, 8);
    }
 
    win32_poll_monitors();
}

file_scope monitor *create_monitor(DISPLAY_DEVICEW *adapter, DISPLAY_DEVICEW *display) {
    DEVMODEW dm;
    zero_memory(&dm, sizeof(dm));
    dm.dmSize = sizeof(dm);

    EnumDisplaySettingsW(adapter->DeviceName, ENUM_CURRENT_SETTINGS, &dm);

    HDC dc = CreateDCW(L"DISPLAY", adapter->DeviceName, null, 0);
    defer(DeleteDC(dc));

    s32 widthMM, heightMM;
    if (IS_WINDOWS_8_POINT_1_OR_GREATER()) {
        widthMM  = GetDeviceCaps(dc, HORZSIZE);
        heightMM = GetDeviceCaps(dc, VERTSIZE);
    } else {
        widthMM  = (s32) (dm.dmPelsWidth * 25.4f / GetDeviceCaps(dc, LOGPIXELSX));
        heightMM = (s32) (dm.dmPelsHeight * 25.4f / GetDeviceCaps(dc, LOGPIXELSY));
    }

    auto *mon     = malloc<monitor>({.Alloc = platform_get_persistent_allocator()});
    mon->WidthMM  = widthMM;
    mon->HeightMM = heightMM;

    wchar_t *name = adapter->DeviceString;
    if (display) name = display->DeviceString;

    mon->Name = platform_utf16_to_utf8(name, platform_get_persistent_allocator());

    if (adapter->StateFlags & DISPLAY_DEVICE_MODESPRUNED) mon->PlatformData.Win32.ModesPruned = true;

    copy_memory(mon->PlatformData.Win32.AdapterName, adapter->DeviceName, c_string_length(adapter->DeviceName) * sizeof(wchar_t));
    WideCharToMultiByte(CP_UTF8, 0, adapter->DeviceName, -1, mon->PlatformData.Win32.PublicAdapterName, sizeof(mon->PlatformData.Win32.PublicAdapterName), null, null);

    if (display) {
        copy_memory(mon->PlatformData.Win32.DisplayName, display->DeviceName, c_string_length(adapter->DeviceName) * sizeof(wchar_t));
        WideCharToMultiByte(CP_UTF8, 0, display->DeviceName, -1, mon->PlatformData.Win32.PublicDisplayName, sizeof(mon->PlatformData.Win32.PublicDisplayName), null, null);
    }

    auto dmPosition = dm.DUMMYUNIONNAME.dmPosition;  // What's up with this?

    RECT rect;
    rect.left   = dmPosition.x;
    rect.top    = dmPosition.y;
    rect.right  = dmPosition.x + dm.dmPelsWidth;
    rect.bottom = dmPosition.y + dm.dmPelsHeight;

    EnumDisplayMonitors(
        null, &rect, [](HMONITOR handle, HDC dc, RECT *rect, LPARAM data) {
            MONITORINFOEXW mi;
            zero_memory(&mi, sizeof(mi));
            mi.cbSize = sizeof(mi);

            if (GetMonitorInfoW(handle, (MONITORINFO *) &mi)) {
                auto *mon = (monitor *) data;
                if (compare_string(mi.szDevice, mon->PlatformData.Win32.AdapterName) == -1) {
                    mon->PlatformData.Win32.hMonitor = handle;
                }
            }
            return 1;  // true
        },
        (LPARAM) mon);

    mon->CurrentMode = monitor_get_current_display_mode(mon);
    return mon;
}

file_scope void do_monitor_event(monitor *mon, monitor_event::action action, bool insertLast) {
    if (action == monitor_event::CONNECTED) {
        PUSH_ALLOC(platform_get_persistent_allocator()) {
            insert_at_index(&Monitors, insertLast ? Monitors.Count : 0, mon);
        }
    } else {
        s64 index = find(Monitors, mon);
        if (index != -1) remove_ordered_at_index(&Monitors, index);

        free(mon->Name.Data);
        free(mon->DisplayModes.Data);
        free(mon);
    }

    emit(&MonitorEvent, monitor_event{mon, action});
}

// Splits a color depth into red, green and blue bit depths
file_scope void split_bpp(s32 bpp, s32 *red, s32 *green, s32 *blue) {
    s32 delta;
    if (bpp == 32) bpp = 24;

    *red = *green = *blue = bpp / 3;
    delta                 = bpp - (*red * 3);
    if (delta >= 1) *green = *green + 1;
    if (delta == 2) *red = *red + 1;
}

display_mode monitor_get_current_display_mode(monitor *mon) {
    DEVMODEW dm;
    zero_memory(&dm, sizeof(dm));
    dm.dmSize = sizeof(dm);

    EnumDisplaySettingsW(mon->PlatformData.Win32.AdapterName, ENUM_CURRENT_SETTINGS, &dm);

    display_mode mode;
    mode.Width  = dm.dmPelsWidth;
    mode.Height = dm.dmPelsHeight;

    mode.RefreshRate = dm.dmDisplayFrequency;
    split_bpp(dm.dmBitsPerPel, &mode.RedBits, &mode.GreenBits, &mode.BlueBits);
    return mode;
}

// Chooses the video mode most closely matching the desired one
file_scope display_mode choose_video_mode(monitor *mon, display_mode desired) {
    u32 sizeDiff, leastSizeDiff   = numeric_info<u32>::max();
    u32 rateDiff, leastRateDiff   = numeric_info<u32>::max();
    u32 colorDiff, leastColorDiff = numeric_info<u32>::max();

    assert(mon->DisplayModes.Count);

    display_mode closest;
    For(mon->DisplayModes) {
        colorDiff = 0;

        if (desired.RedBits != display_mode::DONT_CARE) colorDiff += abs(it.RedBits - desired.RedBits);
        if (desired.GreenBits != display_mode::DONT_CARE) colorDiff += abs(it.GreenBits - desired.GreenBits);
        if (desired.BlueBits != display_mode::DONT_CARE) colorDiff += abs(it.BlueBits - desired.BlueBits);

        sizeDiff = abs((it.Width - desired.Width) * (it.Width - desired.Width) +
                       (it.Height - desired.Height) * (it.Height - desired.Height));

        if (desired.RefreshRate != display_mode::DONT_CARE) {
            rateDiff = abs(it.RefreshRate - desired.RefreshRate);
        } else {
            rateDiff = numeric_info<u32>::max() - it.RefreshRate;
        }

        if ((colorDiff < leastColorDiff) || (colorDiff == leastColorDiff && sizeDiff < leastSizeDiff) ||
            (colorDiff == leastColorDiff && sizeDiff == leastSizeDiff && rateDiff < leastRateDiff)) {
            closest        = it;
            leastSizeDiff  = sizeDiff;
            leastRateDiff  = rateDiff;
            leastColorDiff = colorDiff;
        }
    }
    return closest;
}

s64 monitor_connect_callback(delegate<void(monitor_event)> cb) { return connect(&MonitorEvent, cb); }
bool monitor_disconnect_callback(s64 cb) { return disconnect(&MonitorEvent, cb); }

rect get_work_area(monitor *mon) {
    MONITORINFO mi;
    mi.cbSize = sizeof(MONITORINFO);
    GetMonitorInfoW(mon->PlatformData.Win32.hMonitor, &mi);

    rect result;
    result.top = mi.rcWork.top;
    result.left = mi.rcWork.left;
    result.bottom = mi.rcWork.bottom;
    result.right = mi.rcWork.right;
    return result;
}

bool set_display_mode(monitor *mon, display_mode desired) {
    display_mode best = choose_video_mode(mon, desired);
    if (monitor_get_current_display_mode(mon) == best) return true;

    DEVMODEW dm;
    zero_memory(&dm, sizeof(dm));
    {
        dm.dmSize       = sizeof(dm);
        dm.dmFields     = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL | DM_DISPLAYFREQUENCY;
        dm.dmPelsWidth  = best.Width;
        dm.dmPelsHeight = best.Height;
        dm.dmBitsPerPel = best.RedBits + best.GreenBits + best.BlueBits;
        if (dm.dmBitsPerPel < 15 || dm.dmBitsPerPel >= 24) dm.dmBitsPerPel = 32;
        dm.dmDisplayFrequency = best.RefreshRate;
    }

    auto result = ChangeDisplaySettingsExW(mon->PlatformData.Win32.AdapterName, &dm, null, CDS_FULLSCREEN, null);
    if (result != DISP_CHANGE_SUCCESSFUL) {
        auto *description = "Unknown error";

        if (result == DISP_CHANGE_BADDUALVIEW) description = "The system uses DualView";
        if (result == DISP_CHANGE_BADFLAGS) description = "Invalid flags";
        if (result == DISP_CHANGE_BADMODE) description = "Graphics mode not supported";
        if (result == DISP_CHANGE_BADPARAM) description = "Invalid parameter";
        if (result == DISP_CHANGE_FAILED) description = "Graphics mode failed";
        if (result == DISP_CHANGE_NOTUPDATED) description = "Failed to write to registry";
        if (result == DISP_CHANGE_RESTART) description = "Computer restart required";

        platform_report_error(sprint("Failed to set video mode: {!YELLOW}{}{!}", description));
        return false;
    }

    mon->PlatformData.Win32.ModeChanged = true;
    return true;
}

void restore_display_mode(monitor *mon) {
    if (mon->PlatformData.Win32.ModeChanged) {
        ChangeDisplaySettingsExW(mon->PlatformData.Win32.AdapterName, null, null, CDS_FULLSCREEN, null);
        mon->PlatformData.Win32.ModeChanged = false;
    }
}

// Doesn't add duplicates and doesn't sort _out_
file_scope void get_display_modes(monitor *mon) {
    s32 modeIndex = 0;
    while (true) {
        DEVMODEW dm;
        zero_memory(&dm, sizeof(dm));
        dm.dmSize = sizeof(dm);

        if (!EnumDisplaySettingsW(mon->PlatformData.Win32.AdapterName, modeIndex, &dm)) break;
        modeIndex++;

        // Skip modes with less than 15 BPP
        if (dm.dmBitsPerPel < 15) continue;

        display_mode mode;
        mode.Width       = dm.dmPelsWidth;
        mode.Height      = dm.dmPelsHeight;
        mode.RefreshRate = dm.dmDisplayFrequency;
        split_bpp(dm.dmBitsPerPel, &mode.RedBits, &mode.GreenBits, &mode.BlueBits);

        bool toContinue = false;
        For(mon->DisplayModes) {
            if (it == mode) {
                toContinue = true;
                break;
            }
        }
        if (toContinue) continue;

        if (mon->PlatformData.Win32.ModesPruned) {
            // Skip modes not supported by the connected displays
            if (ChangeDisplaySettingsExW(mon->PlatformData.Win32.AdapterName, &dm, null, CDS_TEST, null) !=
                DISP_CHANGE_SUCCESSFUL) {
                continue;
            }
        }

        if (!mon->DisplayModes.Allocated) make_dynamic(&mon->DisplayModes, 8);
        add(&mon->DisplayModes, mode);
    }

    if (!mon->DisplayModes) {
        // Hack: Report the current mode if no valid modes were found
        add(&mon->DisplayModes, monitor_get_current_display_mode(mon));
    }
}

// Poll for changes in the set of connected monitors
void win32_poll_monitors() {
    array<monitor *> disconnected;
    PUSH_ALLOC(platform_get_persistent_allocator()) {
        disconnected = clone(Monitors);
    }
    defer(free(disconnected.Data));

    DISPLAY_DEVICEW adapter, display;
    for (DWORD adapterIndex = 0;; adapterIndex++) {
        bool insertLast = true;

        zero_memory(&adapter, sizeof(adapter));
        adapter.cb = sizeof(adapter);

        if (!EnumDisplayDevicesW(null, adapterIndex, &adapter, 0)) break;
        if (!(adapter.StateFlags & DISPLAY_DEVICE_ACTIVE)) continue;
        if (adapter.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) insertLast = false;

        DWORD displayIndex = 0;
        while (true) {
            zero_memory(&display, sizeof(display));
            display.cb = sizeof(display);

            if (!EnumDisplayDevicesW(adapter.DeviceName, displayIndex, &display, 0)) break;
            if (!(display.StateFlags & DISPLAY_DEVICE_ACTIVE)) continue;

            bool toContinue = false;
            For_as(index, range(disconnected.Count)) {
                auto *it = disconnected[index];
                if (it && compare_string(it->PlatformData.Win32.DisplayName, display.DeviceName) == -1) {
                    *(disconnected.Data + index) = null;
                    toContinue                   = true;
                    break;
                }
            }
            if (toContinue) continue;

            do_monitor_event(create_monitor(&adapter, &display), monitor_event::CONNECTED, insertLast);

            insertLast = true;
            ++displayIndex;
        }

        if (displayIndex == 0) {
            bool toContinue = false;
            For_as(index, range(disconnected.Count)) {
                auto *it = disconnected[index];
                if (it && compare_string(it->PlatformData.Win32.AdapterName, adapter.DeviceName) == -1) {
                    *(disconnected.Data + index) = null;
                    toContinue              = true;
                    break;
                }
            }
            if (toContinue) continue;

            do_monitor_event(create_monitor(&adapter, null), monitor_event::CONNECTED, insertLast);
        }
    }

    For(disconnected) {
        if (it) do_monitor_event(it, monitor_event::DISCONNECTED, false);
    }

    For(Monitors) {
        PUSH_ALLOC(platform_get_persistent_allocator()) {
            get_display_modes(it);
        }
        quick_sort(it->DisplayModes.Data, it->DisplayModes.Count);
    }
}

void poll_monitors() { win32_poll_monitors(); }

array<monitor *> get_monitors() { return Monitors; }

monitor *get_primary_monitor() { return Monitors[0]; }

monitor *monitor_from_window(window win) {
    HMONITOR hMonitor = MonitorFromWindow((HWND) win, MONITOR_DEFAULTTONEAREST);

    monitor *result = null;
    For(Monitors) {
        if (hMonitor == it->PlatformData.Win32.hMonitor) {
            result = it;
            break;
        }
    }
    assert(result);
    return result;
}

int2 get_pos(monitor *mon) {
    DEVMODEW dm;
    zero_memory(&dm, sizeof(dm));
    dm.dmSize = sizeof(dm);

    EnumDisplaySettingsExW(mon->PlatformData.Win32.AdapterName, ENUM_CURRENT_SETTINGS, &dm, EDS_ROTATEDMODE);

    return {dm.DUMMYUNIONNAME.dmPosition.x, dm.DUMMYUNIONNAME.dmPosition.y};  // What's up with DUMMYUNIONNAME
}

float2 get_content_scale(monitor *mon) {
    HMONITOR handle = mon->PlatformData.Win32.hMonitor;

    u32 xdpi, ydpi;
    if (IS_WINDOWS_8_POINT_1_OR_GREATER()) {
        GetDpiForMonitor(handle, MDT_EFFECTIVE_DPI, &xdpi, &ydpi);
    } else {
        const HDC dc = GetDC(null);
        xdpi         = GetDeviceCaps(dc, LOGPIXELSX);
        ydpi         = GetDeviceCaps(dc, LOGPIXELSY);
        ReleaseDC(null, dc);
    }
    return {xdpi / (f32) USER_DEFAULT_SCREEN_DPI, ydpi / (f32) USER_DEFAULT_SCREEN_DPI};
}

#endif  // OS == WINDOWS