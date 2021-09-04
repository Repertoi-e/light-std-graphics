#include "driver.h"

#if not defined BUILDING_DRIVER
#error Error
#endif

#include <lstd/parse.h>

import os;
import fmt;
import path;

// These can be modified with command line arguments.
s64 MemoryInBytes = 64_MiB;
u32 WindowWidth = 800, WindowHeight = 400, TargetFPS = 60;

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

string DLLFileName = TOSTRING(VEHICLE) ".dll";

allocator PersistentAlloc;  // Imgui uses this, it needs to be global

dynamic_library DLL;
update_and_render_func *UpdateAndRender = null;
main_window_event_func *MainWindowEvent = null;

string DLLFile, BuildLockFile;

file_scope void setup_paths() {
    assert(DLLFileName != "");

    // This may be different from the working directory
    string exeDir = path_directory(os_get_current_module());

    DLLFile       = path_join(exeDir, DLLFileName);
    BuildLockFile = path_join(exeDir, "buildlock");
}

// @TODO: This fails in Dist configuration for some reason
file_scope bool reload_code() {
    UpdateAndRender = null;
    MainWindowEvent = null;
    if (DLL) free(DLL);

    string copyPath = path_join(path_directory(DLLFile), "loaded_code.dll");
    if (!path_copy(DLLFile, copyPath, true)) {
        print("Error: Couldn't write to {!YELLOW}\"{}\"{!}. App already running?\n", copyPath);
        return false;
    }

    if (!(DLL = os_dynamic_library_load(copyPath))) {
        print("Error: Couldn't load {!YELLOW}\"{}\"{!} (copied from {!GRAY}\"{}\"{}) as the code for the engine.\n", copyPath, DLLFile);
        return false;
    }

    UpdateAndRender = (update_and_render_func *) os_dynamic_library_get_symbol(DLL, "update_and_render");
    if (!UpdateAndRender) {
        print("Error: Couldn't load update_and_render\n");
        return false;
    }

    MainWindowEvent = (main_window_event_func *) os_dynamic_library_get_symbol(DLL, "main_window_event");
    if (!MainWindowEvent) {
        print("Error: Couldn't load main_window_event\n");
        return false;
    }
    return true;
}

// Returns true if the code was reloaded
file_scope bool check_for_dll_change() {
    static time_t checkTimer = 0, lastTime = 0;

    if (!path_exists(BuildLockFile) && (checkTimer % 20 == 0)) {
        auto writeTime = path_last_modification_time(DLLFile);
        if (writeTime != lastTime) {
            lastTime = writeTime;
            return reload_code();
        }
    }
    ++checkTimer;
    return false;
}

void init_imgui_for_our_windows(window *mainWindow);
void imgui_for_our_windows_new_frame();

// @TODO: Provide a library construct for parsing arguments automatically
file_scope void parse_arguments() {
    array<string> usage;
    array_append(usage, string("Usage:\n"));
    array_append(usage,
                 string("    {!YELLOW}-dll <name>{!GRAY}    "
                        "Specifies which dll to hot load in the engine. By default it searches for cars.dll{!}\n\n"));
    array_append(usage,
                 string("    {!YELLOW}-memory <amount>{!GRAY}    "
                        "Specifies the amount of memory (in MiB) which gets reserved for the app (default is 128 MiB)."
                        "Usage must never cross this.{!}\n\n"));
    array_append(usage,
                 string("    {!YELLOW}-width <value>{!GRAY}    "
                        "Specifies the width of the app window (default is 1200).{!}\n\n"));
    array_append(usage,
                 string("    {!YELLOW}-heigth <value>{!GRAY}    "
                        "Specifies the height of the app window (default is 600).\n\n"));
    array_append(usage,
                 string("    {!YELLOW}-fps <value>{!GRAY}    "
                        "Specifies the target fps (default is 60).\n\n"));
    defer(free(usage));

    bool seekFileName = false,
         seekMemory = false, seekWidth = false, seekHeight = false, seekFPS = false;
    auto args = os_get_command_line_arguments();
    For(args) {
        if (seekFileName) {
            DLLFileName  = it;
            seekFileName = false;
            continue;
        }
        if (seekMemory) {
            auto [value, status, rest] = parse_int<u32, parse_int_options{.ParseSign = false, .LookForBasePrefix = true}>(it, 10);

            if (status == PARSE_INVALID) {
                print(">>> {!RED}Invalid character encountered when parsing memory argument. The argument was: \"{}\"\n", it);
            } else if (status == PARSE_TOO_MANY_DIGITS) {
                print(">>> {!RED}Memory value too big. The argument was: \"{}\"\n", it);
            } else if (status == PARSE_EXHAUSTED) {
                print(">>> {!RED}Couldn't parse memory value. The argument was: \"{}\"\n", it);
            } else {
                MemoryInBytes = (s64) value * 1_MiB;
            }
            seekMemory = false;
            continue;
        }
        if (seekWidth) {
            auto [value, status, rest] = parse_int<u32, parse_int_options{.ParseSign = false, .LookForBasePrefix = true}>(it, 10);

            if (status == PARSE_INVALID) {
                print(">>> {!RED}Invalid character encountered when parsing memory argument. The argument was: \"{}\"\n", it);
            } else if (status == PARSE_TOO_MANY_DIGITS) {
                print(">>> {!RED}Memory value too big. The argument was: \"{}\"\n", it);
            } else if (status == PARSE_EXHAUSTED) {
                print(">>> {!RED}Couldn't parse memory value. The argument was: \"{}\"\n", it);
            } else {
                WindowWidth = value;
            }
            seekWidth = false;
            continue;
        }
        if (seekHeight) {
            auto [value, status, rest] = parse_int<u32, parse_int_options{.ParseSign = false, .LookForBasePrefix = true}>(it, 10);

            if (status == PARSE_INVALID) {
                print(">>> {!RED}Invalid character encountered when parsing memory argument. The argument was: \"{}\"\n", it);
            } else if (status == PARSE_TOO_MANY_DIGITS) {
                print(">>> {!RED}Memory value too big. The argument was: \"{}\"\n", it);
            } else if (status == PARSE_EXHAUSTED) {
                print(">>> {!RED}Couldn't parse memory value. The argument was: \"{}\"\n", it);
            } else {
                WindowHeight = value;
            }
            seekHeight = false;
            continue;
        }
        if (seekFPS) {
            auto [value, status, rest] = parse_int<u32, parse_int_options{.ParseSign = false, .LookForBasePrefix = true}>(it, 10);

            if (status == PARSE_INVALID) {
                print(">>> {!RED}Invalid character encountered when parsing memory argument. The argument was: \"{}\"\n", it);
            } else if (status == PARSE_TOO_MANY_DIGITS) {
                print(">>> {!RED}Memory value too big. The argument was: \"{}\"\n", it);
            } else if (status == PARSE_EXHAUSTED) {
                print(">>> {!RED}Couldn't parse memory value. The argument was: \"{}\"\n", it);
            } else {
                TargetFPS = value;
            }
            seekFPS = false;
            continue;
        }
        if (it == "-dll") {
            seekFileName = true;
        } else if (it == "-memory") {
            seekMemory = true;
        } else if (it == "-width") {
            seekWidth = true;
        } else if (it == "-height") {
            seekHeight = true;
        } else if (it == "-fps") {
            seekFPS = true;
        } else {
            print(">>> {!RED}Encountered invalid argument (\"{}\").{!}\n", it);
            For(usage) print(it);
            break;
        }
    }
    if (seekFileName) {
        print(">>> {!RED}Invalid use of \"-dll\" argument. Expected a parameter.{!}\n");
        For(usage) print(it);
    }
    if (seekMemory) {
        print(">>> {!RED}Invalid use of \"-memory\" argument. Expected a parameter.{!}\n");
        For(usage) print(it);
    }
    if (seekWidth) {
        print(">>> {!RED}Invalid use of \"-width\" argument. Expected a parameter.{!}\n");
        For(usage) print(it);
    }
    if (seekHeight) {
        print(">>> {!RED}Invalid use of \"-height\" argument. Expected a parameter.{!}\n");
        For(usage) print(it);
    }
    if (seekFPS) {
        print(">>> {!RED}Invalid use of \"-fps\" argument. Expected a parameter.{!}\n");
        For(usage) print(it);
    }
}

s32 main() {
    // We allocate all the state we need next to each other in order to reduce fragmentation.
    auto [data, m, g, pool] = os_allocate_packed<tlsf_allocator_data, memory, graphics>(MemoryInBytes);
    PersistentAlloc         = {tlsf_allocator, data};
    allocator_add_pool(PersistentAlloc, pool, MemoryInBytes);

    allocator_add_pool(Context.TempAlloc, os_allocate_block(1_MiB), 1_MiB);

    Memory   = m;
    Graphics = g;

    auto newContext  = Context;
    newContext.Log   = &cout;
    newContext.Alloc = PersistentAlloc;
    OVERRIDE_CONTEXT(newContext);

    m->Alloc     = PersistentAlloc;
    m->TempAlloc = Context.TempAlloc;

    parse_arguments();
    setup_paths();

    // string windowTitle = sprint("Graphics Engine | {}", DLLFileName);
    string windowTitle = "Calculator";

    auto windowFlags = window::SHOWN | window::RESIZABLE | window::VSYNC | window::FOCUS_ON_SHOW | window::CLOSE_ON_ALT_F4;
    m->MainWindow    = allocate<window>()->init(windowTitle, window::DONT_CARE, window::DONT_CARE, WindowWidth, WindowHeight, windowFlags);

    auto icon = pixel_buffer("data/calc.png", false, pixel_format::RGBA);
    array<pixel_buffer> icons;
    array_append(icons, icon);
    m->MainWindow->set_icon(icons);
    free(icons);

    m->MainWindow->Event.connect(delegate<bool(const event &e)>([](auto e) {
        if (MainWindowEvent) return MainWindowEvent(e);
        return false;
    }));

    m->GetStateImpl = [](const string &name, s64 size, bool *created) {
        string identifier = name;
        string_append(identifier, "Ident");

        auto **found = find(Memory->States, identifier).Value;
        if (!found) {
            // We allocate with alignment 16 because we use SIMD types
            void *result = allocate_array<char>(size, {.Alignment = 16});
            add(Memory->States, identifier, result);
            *created = true;
            return result;
        }

        free(identifier);
        return *found;
    };

    // @TODO: TargetFPS currently does nothing, we rely on g.swap() and vsync to hit target monitor refresh rate

    // This is affecting any physics time steps though
    m->FrameDelta = 1.0f / TargetFPS;

    g->init(graphics_api::Direct3D);
    g->set_blend(true);
    g->set_depth_testing(false);

    ImGui::SetAllocatorFunctions([](size_t size, void *) { return (void *) allocate_array<char>(size, {.Alloc = PersistentAlloc}); },
                                 [](void *ptr, void *) { ::lstd::free(ptr); });  // Without the namespace this selects the CRT free for some bizarre reason...

    init_imgui_for_our_windows(m->MainWindow);
    m->ImGuiContext = ImGui::GetCurrentContext();
    exit_schedule(delegate<void(void)>([]() {
        ImGui::DestroyPlatformWindows();
        ImGui::DestroyContext();
    }));

    // This state also needs to be shared
#if defined DEBUG_MEMORY
    m->DEBUG_memory = DEBUG_memory;
#endif

    imgui_renderer imguiRenderer;
    imguiRenderer.init(g);
    defer(imguiRenderer.release());

    while (true) {
        m->ReloadedThisFrame = check_for_dll_change();
        if (m->RequestReloadNextFrame) {
            m->ReloadedThisFrame      = reload_code();
            m->RequestReloadNextFrame = false;
        }

        window::update();
        if (m->MainWindow->IsDestroying) break;

        imgui_for_our_windows_new_frame();
        ImGui::NewFrame();
        if (UpdateAndRender) UpdateAndRender(Memory, Graphics);
        ImGui::Render();

        if (m->MainWindow->is_visible()) {
            g->set_target_window(m->MainWindow);
            g->set_cull_mode(cull::None);
            imguiRenderer.draw(ImGui::GetDrawData());
            g->swap();
        }

        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault(null, &imguiRenderer);
        }
    }
}

struct imgui_our_windows_data {
    window *Window      = null;
    window *MouseWindow = null;

    s64 Time = 0;

    bool MouseJustPressed[Mouse_Button_Last + 1]{};
    window *KeyOwnerWindows[Key_Last + 1]{};

    bool InstalledCallbacks = false;
    bool WantUpdateMonitors = false;
};

struct imgui_our_windows_viewport_data {
    window *Window                 = null;
    bool WindowOwned               = false;
    s32 IgnoreWindowPosEventFrame  = -1;
    s32 IgnoreWindowSizeEventFrame = -1;
};

// This can be global
file_scope cursor MouseCursors[ImGuiMouseCursor_COUNT] = {
    cursor(OS_ARROW), cursor(OS_IBEAM), cursor(OS_RESIZE_ALL), cursor(OS_RESIZE_NS), cursor(OS_RESIZE_WE), cursor(OS_RESIZE_NESW), cursor(OS_RESIZE_NWSE), cursor(OS_HAND)};

file_scope void update_modifiers() {
    ImGuiIO &io = ImGui::GetIO();

    // Modifiers are not reliable across systems
    io.KeyCtrl  = io.KeysDown[Key_LeftControl] || io.KeysDown[Key_RightControl];
    io.KeyShift = io.KeysDown[Key_LeftShift] || io.KeysDown[Key_RightShift];
    io.KeyAlt   = io.KeysDown[Key_LeftAlt] || io.KeysDown[Key_RightAlt];
#if PLATFORM == WINDOWS
    io.KeySuper = false;
#else
    io.KeySuper = io.KeysDown[Key_LeftGUI] || io.KeysDown[Key_RightGUI];
#endif
}

file_scope bool common_event_callback(const event &e) {
    ImGuiIO &io = ImGui::GetIO();
    auto *bd    = (imgui_our_windows_data *) io.BackendPlatformUserData;

    if (e.Type == event::Keyboard_Pressed) {
        io.KeysDown[e.KeyCode]         = true;
        bd->KeyOwnerWindows[e.KeyCode] = e.Window;
        update_modifiers();
    } else if (e.Type == event::Keyboard_Released) {
        io.KeysDown[e.KeyCode]         = false;
        bd->KeyOwnerWindows[e.KeyCode] = null;
        update_modifiers();
    } else if (e.Type == event::Code_Point_Typed) {
        io.AddInputCharacter(e.CP);
    } else if (e.Type == event::Mouse_Button_Pressed) {
        bd->MouseJustPressed[e.Button] = true;
    } else if (e.Type == event::Mouse_Wheel_Scrolled) {
        io.MouseWheelH += e.ScrollX;
        io.MouseWheel += e.ScrollY;
    } else if (e.Type == event::Window_Focused) {
        io.AddFocusEvent(e.Focused);
    } else if (e.Type == event::Mouse_Entered_Window) {
        bd->MouseWindow = e.Window;
    } else if (e.Type == event::Mouse_Left_Window) {
        bd->MouseWindow = null;
    }

    return false;
}

#include <lstd/common/windows.h>

// We provide a Win32 implementation because this is such a common issue for IME users
#if OS == WINDOWS && !defined IMGUI_DISABLE_WIN32_FUNCTIONS && !defined IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS
#define HAS_WIN32_IME 1
#if COMPILER == MSVC
#pragma comment(lib, "imm32")
#endif
file_scope void imgui_set_ime_pos(ImGuiViewport *viewport, ImVec2 pos) {
    COMPOSITIONFORM cf = {
        CFS_FORCE_POSITION, {(s32) (pos.x - viewport->Pos.x), (s32) (pos.y - viewport->Pos.y)}, {0, 0, 0, 0}};
    if (HWND hWnd = (HWND) viewport->PlatformHandleRaw) {
        if (HIMC himc = ImmGetContext(hWnd)) {
            ImmSetCompositionWindow(himc, &cf);
            ImmReleaseContext(hWnd, himc);
        }
    }
}
#else
#define HAS_WIN32_IME 0
#endif

file_scope void imgui_update_monitors() {
    auto *bd = (imgui_our_windows_data *) ImGui::GetIO().BackendPlatformUserData;

    ImGuiPlatformIO &platformIO = ImGui::GetPlatformIO();

    platformIO.Monitors.resize(0);

    os_poll_monitors();
    For(os_get_monitors()) {
        vec2<s32> pos    = os_get_monitor_pos(it);
        auto displayMode = os_get_current_display_mode(it);

        ImGuiPlatformMonitor monitor;
        monitor.MainPos  = ImVec2((f32) pos.x, (f32) pos.y);
        monitor.MainSize = ImVec2((f32) displayMode.Width, (f32) displayMode.Height);

        rect workArea    = os_get_work_area(it);
        monitor.WorkPos  = ImVec2((f32) workArea.Top, (f32) workArea.Left);
        monitor.WorkSize = ImVec2((f32) workArea.width(), (f32) workArea.height());

        v2 scale         = os_get_monitor_content_scale(it);
        monitor.DpiScale = scale.x;

        platformIO.Monitors.push_back(monitor);
    }

    bd->WantUpdateMonitors = false;
}

// Slightly modified version of "Photoshop" theme by @Derydoca (https://github.com/ocornut/imgui/issues/707)
file_scope void imgui_init_photoshop_style() {
    ImGuiStyle *style = &ImGui::GetStyle();

    ImVec4 *colors                         = style->Colors;
    colors[ImGuiCol_Text]                  = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
    colors[ImGuiCol_TextDisabled]          = ImVec4(0.500f, 0.500f, 0.500f, 1.000f);
    colors[ImGuiCol_WindowBg]              = ImVec4(0.180f, 0.180f, 0.180f, 1.000f);
    colors[ImGuiCol_ChildBg]               = ImVec4(0.200f, 0.200f, 0.200f, 1.000f);
    colors[ImGuiCol_PopupBg]               = ImVec4(0.313f, 0.313f, 0.313f, 1.000f);
    colors[ImGuiCol_Border]                = ImVec4(0.266f, 0.266f, 0.266f, 1.000f);
    colors[ImGuiCol_BorderShadow]          = ImVec4(0.000f, 0.000f, 0.000f, 0.000f);
    colors[ImGuiCol_FrameBg]               = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
    colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.200f, 0.200f, 0.200f, 1.000f);
    colors[ImGuiCol_FrameBgActive]         = ImVec4(0.280f, 0.280f, 0.280f, 1.000f);
    colors[ImGuiCol_TitleBg]               = ImVec4(0.09f, 0.09f, 0.09f, 1.000f);
    colors[ImGuiCol_TitleBgActive]         = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
    colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.118f, 0.118f, 0.118f, 1.000f);
    colors[ImGuiCol_MenuBarBg]             = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
    colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
    colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.277f, 0.277f, 0.277f, 1.000f);
    colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.300f, 0.300f, 0.300f, 1.000f);
    colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_CheckMark]             = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
    colors[ImGuiCol_SliderGrab]            = ImVec4(0.391f, 0.391f, 0.391f, 1.000f);
    colors[ImGuiCol_SliderGrabActive]      = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_Button]                = ImVec4(1.000f, 1.000f, 1.000f, 0.000f);
    colors[ImGuiCol_ButtonHovered]         = ImVec4(1.000f, 1.000f, 1.000f, 0.156f);
    colors[ImGuiCol_ButtonActive]          = ImVec4(1.000f, 1.000f, 1.000f, 0.391f);
    colors[ImGuiCol_Header]                = ImVec4(0.313f, 0.313f, 0.313f, 1.000f);
    colors[ImGuiCol_HeaderHovered]         = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
    colors[ImGuiCol_HeaderActive]          = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
    colors[ImGuiCol_Separator]             = colors[ImGuiCol_Border];
    colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.391f, 0.391f, 0.391f, 1.000f);
    colors[ImGuiCol_SeparatorActive]       = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_ResizeGrip]            = ImVec4(1.000f, 1.000f, 1.000f, 0.250f);
    colors[ImGuiCol_ResizeGripHovered]     = ImVec4(1.000f, 1.000f, 1.000f, 0.670f);
    colors[ImGuiCol_ResizeGripActive]      = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_Tab]                   = ImVec4(0.098f, 0.098f, 0.098f, 1.000f);
    colors[ImGuiCol_TabHovered]            = ImVec4(0.352f, 0.352f, 0.352f, 1.000f);
    colors[ImGuiCol_TabActive]             = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
    colors[ImGuiCol_TabUnfocused]          = ImVec4(0.098f, 0.098f, 0.098f, 1.000f);
    colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
    colors[ImGuiCol_DockingPreview]        = ImVec4(1.000f, 0.391f, 0.000f, 0.781f);
    colors[ImGuiCol_DockingEmptyBg]        = ImVec4(0.180f, 0.180f, 0.180f, 1.000f);
    colors[ImGuiCol_PlotLines]             = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
    colors[ImGuiCol_PlotLinesHovered]      = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_PlotHistogram]         = ImVec4(0.586f, 0.586f, 0.586f, 1.000f);
    colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_TextSelectedBg]        = ImVec4(1.000f, 1.000f, 1.000f, 0.156f);
    colors[ImGuiCol_DragDropTarget]        = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_NavHighlight]          = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.000f, 0.000f, 0.000f, 0.586f);
    colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.000f, 0.000f, 0.000f, 0.586f);

    style->ChildRounding     = 4.0f;
    style->FrameBorderSize   = 1.0f;
    style->FrameRounding     = 2.0f;
    style->GrabMinSize       = 7.0f;
    style->PopupRounding     = 2.0f;
    style->ScrollbarRounding = 12.0f;
    style->ScrollbarSize     = 13.0f;
    style->TabBorderSize     = 1.0f;
    style->TabRounding       = 0.0f;
    style->WindowRounding    = 4.0f;
}

file_scope void init_imgui_for_our_windows(window *mainWindow) {
    // os_poll_monitors();  // @Cleanup: In the ideal case we shouldn't need this.

    ImGui::CreateContext();

    ImGuiIO &io = ImGui::GetIO();
    io.Fonts    = allocate<ImFontAtlas>();

    auto *bd = allocate<imgui_our_windows_data>();

    io.BackendPlatformUserData = (void *) bd;
    io.BackendPlatformName     = "imgui_impl_lstd_graphics";
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;       // We can honor GetMouseCursor() values (optional)
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;        // We can honor io.WantSetMousePos requests (optional, rarely used)
    io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;  // We can create multi-viewports on the Platform side (optional)
    io.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport;

    bd->Window             = mainWindow;
    bd->WantUpdateMonitors = true;

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    // io.ConfigViewportsNoAutoMerge = true;
    // io.ConfigViewportsNoTaskBarIcon = true;
    // io.ConfigViewportsNoDefaultParent = true;
    // io.ConfigDockingAlwaysTabBar = true;
    // io.ConfigDockingTransparentPayload = true;

    // We tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle &style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding              = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGui::StyleColorsDark();
    imgui_init_photoshop_style();

    io.SetClipboardTextFn = [](auto, const char *text) { os_set_clipboard_content(string(text)); };
    io.GetClipboardTextFn = [](auto) {
        static array<char> buffer;

        // ImGui expects a null-terminated string.
        // @TODO @Speed Make os_get_clipboard_content() ensure it is null terminated to avoid allocating. Similar to os_get_env.

        auto content = os_get_clipboard_content();
        array_reserve(buffer, content.Count + 1);
        copy_memory(buffer.Data, content.Data, content.Count);
        buffer.Count          = content.Count + 1;
        buffer[content.Count] = 0;

        return (const char *) buffer.Data;
    };

    io.KeyMap[ImGuiKey_Tab]         = Key_Tab;
    io.KeyMap[ImGuiKey_LeftArrow]   = Key_Left;
    io.KeyMap[ImGuiKey_RightArrow]  = Key_Right;
    io.KeyMap[ImGuiKey_UpArrow]     = Key_Up;
    io.KeyMap[ImGuiKey_DownArrow]   = Key_Down;
    io.KeyMap[ImGuiKey_PageUp]      = Key_PageUp;
    io.KeyMap[ImGuiKey_PageDown]    = Key_PageDown;
    io.KeyMap[ImGuiKey_Home]        = Key_Home;
    io.KeyMap[ImGuiKey_End]         = Key_End;
    io.KeyMap[ImGuiKey_Insert]      = Key_Insert;
    io.KeyMap[ImGuiKey_Delete]      = Key_Delete;
    io.KeyMap[ImGuiKey_Backspace]   = Key_Backspace;
    io.KeyMap[ImGuiKey_Space]       = Key_Space;
    io.KeyMap[ImGuiKey_Enter]       = Key_Enter;
    io.KeyMap[ImGuiKey_Escape]      = Key_Escape;
    io.KeyMap[ImGuiKey_KeyPadEnter] = KeyPad_Enter;
    io.KeyMap[ImGuiKey_A]           = Key_A;
    io.KeyMap[ImGuiKey_C]           = Key_C;
    io.KeyMap[ImGuiKey_V]           = Key_V;
    io.KeyMap[ImGuiKey_X]           = Key_X;
    io.KeyMap[ImGuiKey_Y]           = Key_Y;
    io.KeyMap[ImGuiKey_Z]           = Key_Z;

    bd->InstalledCallbacks = true;
    mainWindow->Event.connect(common_event_callback);

    auto *vd        = allocate<imgui_our_windows_viewport_data>();
    vd->Window      = bd->Window;  // == mainWindow
    vd->WindowOwned = false;

    ImGuiViewport *mainViewport    = ImGui::GetMainViewport();
    mainViewport->PlatformHandle   = (void *) bd->Window;  // == mainWindow
    mainViewport->PlatformUserData = vd;
#if OS == WINDOWS
    mainViewport->PlatformHandleRaw = mainWindow->PlatformData.Win32.hWnd;
#endif

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGuiPlatformIO &platformIO = ImGui::GetPlatformIO();

        // Create platform window
        platformIO.Platform_CreateWindow = [](auto *viewport) {
            u32 flags = window::RESIZABLE | window::VSYNC | window::MOUSE_PASS_THROUGH;
            // @TODO: Window size render target stuff is wrong when we have borders (comment the line below)
            if (viewport->Flags & ImGuiViewportFlags_NoDecoration) flags |= window::BORDERLESS;
            if (viewport->Flags & ImGuiViewportFlags_TopMost) flags |= window::ALWAYS_ON_TOP;

            auto width  = (s32) viewport->Size.x;
            auto height = (s32) viewport->Size.y;

            auto *win = allocate<window>()->init("", window::DONT_CARE, window::DONT_CARE, width, height, flags);

            auto *bd = (imgui_our_windows_data *) ImGui::GetIO().BackendPlatformUserData;
            auto *vd = allocate<imgui_our_windows_viewport_data>();

            vd->Window      = win;
            vd->WindowOwned = true;

            viewport->PlatformUserData = vd;
            viewport->PlatformHandle   = (void *) win;
#if OS == WINDOWS
            viewport->PlatformHandleRaw = win->PlatformData.Win32.hWnd;
#endif
            win->set_pos((s32) viewport->Pos.x, (s32) viewport->Pos.y);

            win->Event.connect(common_event_callback);
            win->Event.connect(delegate<bool(const event &)>([](auto e) {
                ImGuiIO &io = ImGui::GetIO();

                auto *viewport = ImGui::FindViewportByPlatformHandle(e.Window);
                if (e.Type == event::Window_Closed) {
                    viewport->PlatformRequestClose = true;
                } else if (e.Type == event::Window_Moved) {
                    viewport->PlatformRequestMove = true;
                } else if (e.Type == event::Window_Resized) {
                    viewport->PlatformRequestResize = true;
                }
                return false;
            }));
        };

        platformIO.Platform_DestroyWindow = [](auto *viewport) {
            auto *bd = (imgui_our_windows_data *) ImGui::GetIO().BackendPlatformUserData;
            auto *vd = (imgui_our_windows_viewport_data *) viewport->PlatformUserData;

            if (vd) {
                if (vd->WindowOwned) {
                    // Release any keys that were pressed in the window being destroyed and are still held down,
                    // because we will not receive any release events after window is destroyed.
                    For(range(Key_Last + 1)) {
                        if (bd->KeyOwnerWindows[it] == vd->Window) {
                            event e;
                            e.Type    = event::Keyboard_Released;
                            e.KeyCode = (u32) it;

                            common_event_callback(e);
                        }
                    }
                }
                free(vd->Window);
                free(vd);
            }
            viewport->PlatformHandle = viewport->PlatformUserData = null;
        };

        platformIO.Platform_ShowWindow = [](auto *viewport) {
#if OS == WINDOWS
            // @Hack Hide icon from task bar
            HWND hwnd = (HWND) viewport->PlatformHandleRaw;
            if (viewport->Flags & ImGuiViewportFlags_NoTaskBarIcon) {
                LONG ex_style = ::GetWindowLongW(hwnd, GWL_EXSTYLE);
                ex_style &= ~WS_EX_APPWINDOW;
                ex_style |= WS_EX_TOOLWINDOW;
                ::SetWindowLongW(hwnd, GWL_EXSTYLE, ex_style);
            }
#endif
            ((window *) viewport->PlatformHandle)->show();
        };

        platformIO.Platform_SetWindowPos = [](auto *viewport, ImVec2 pos) {
            ((window *) viewport->PlatformHandle)->set_pos((s32) pos.x, (s32) pos.y);
        };

        platformIO.Platform_GetWindowPos = [](auto *viewport) {
            auto pos = ((window *) viewport->PlatformHandle)->get_pos();
            return ImVec2((f32) pos.x, (f32) pos.y);
        };

        platformIO.Platform_SetWindowSize = [](auto *viewport, ImVec2 size) {
            ((window *) viewport->PlatformHandle)->set_size((s32) size.x, (s32) size.y);
        };

        platformIO.Platform_GetWindowSize = [](auto *viewport) {
            auto size = ((window *) viewport->PlatformHandle)->get_size();
            return ImVec2((f32) size.x, (f32) size.y);
        };

        platformIO.Platform_SetWindowFocus = [](auto *viewport) { ((window *) viewport->PlatformHandle)->focus(); };
        platformIO.Platform_GetWindowFocus = [](auto *viewport) {
            return (bool) (((window *) viewport->PlatformHandle)->Flags & window::FOCUSED);
        };

        platformIO.Platform_GetWindowMinimized = [](auto *viewport) {
            return (bool) (((window *) viewport->PlatformHandle)->Flags & window::MINIMIZED);
        };

        platformIO.Platform_SetWindowTitle = [](auto *viewport, const char *title) {
            ((window *) viewport->PlatformHandle)->set_title(string(title));
        };

        platformIO.Platform_RenderWindow = [](auto *viewport, auto) {
            Graphics->set_target_window((window *) viewport->PlatformHandle);
        };

        platformIO.Platform_SwapBuffers = [](auto *viewport, auto) { Graphics->swap(); };

        platformIO.Platform_SetWindowAlpha = [](auto *viewport, f32 alpha) {
            ((window *) viewport->PlatformHandle)->set_opacity(alpha);
        };
#if HAS_WIN32_IME
        platformIO.Platform_SetImeInputPos = imgui_set_ime_pos;
#endif

        g_MonitorEvent.connect(delegate<void(const monitor_event &)>([](auto e) {
            auto *bd               = (imgui_our_windows_data *) ImGui::GetIO().BackendPlatformUserData;
            bd->WantUpdateMonitors = true;
        }));
    }
}

file_scope u32 set_bit(u32 number, u32 mask, bool enabled) {
    return number & ~mask | mask & (enabled ? 0xFFFFFFFF : 0);
}

file_scope void imgui_for_our_windows_new_frame() {
    auto *bd = (imgui_our_windows_data *) ImGui::GetIO().BackendPlatformUserData;

    ImGuiIO &io = ImGui::GetIO();
    assert(io.Fonts->IsBuilt());

    auto *mainWindow = bd->Window;

    vec2<s32> windowSize = mainWindow->get_size();
    s32 w = windowSize.x, h = windowSize.y;

    vec2<s32> frameBufferSize = mainWindow->get_framebuffer_size();
    io.DisplaySize            = ImVec2((f32) w, (f32) h);
    if (w > 0 && h > 0) io.DisplayFramebufferScale = ImVec2((f32) frameBufferSize.x / w, (f32) frameBufferSize.y / h);

    if (bd->WantUpdateMonitors) imgui_update_monitors();

    time_t now   = os_get_time();
    io.DeltaTime = bd->Time > 0 ? (f32) os_time_to_seconds(now - bd->Time) : (1.0f / 60.0f);
    bd->Time     = now;

    ImVec2 mousePosPrev     = io.MousePos;
    io.MousePos             = ImVec2(-FLT_MAX, -FLT_MAX);
    io.MouseHoveredViewport = 0;

    // Update mouse buttons
    // (if a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame)
    io.MouseDown[0] = bd->MouseJustPressed[Mouse_Button_Left] || mainWindow->MouseButtons[Mouse_Button_Left];
    io.MouseDown[1] = bd->MouseJustPressed[Mouse_Button_Right] || mainWindow->MouseButtons[Mouse_Button_Right];
    io.MouseDown[2] = bd->MouseJustPressed[Mouse_Button_Middle] || mainWindow->MouseButtons[Mouse_Button_Middle];
    io.MouseDown[3] = bd->MouseJustPressed[Mouse_Button_X1] || mainWindow->MouseButtons[Mouse_Button_X1];
    io.MouseDown[4] = bd->MouseJustPressed[Mouse_Button_X2] || mainWindow->MouseButtons[Mouse_Button_X2];

    zero_memory(bd->MouseJustPressed, sizeof(bd->MouseJustPressed));

    ImGuiPlatformIO &platformIO = ImGui::GetPlatformIO();

    For(platformIO.Viewports) {
        window *win = (window *) it->PlatformHandle;

        bool focused        = win->Flags & window::FOCUSED;
        window *mouseWindow = (bd->MouseWindow == win || focused) ? win : null;

        // Update mouse buttons
        if (focused) {
            io.MouseDown[0] = win->MouseButtons[Mouse_Button_Left];
            io.MouseDown[1] = win->MouseButtons[Mouse_Button_Right];
            io.MouseDown[2] = win->MouseButtons[Mouse_Button_Middle];
            io.MouseDown[3] = win->MouseButtons[Mouse_Button_X1];
            io.MouseDown[4] = win->MouseButtons[Mouse_Button_X2];
        }

        // Set OS mouse position from Dear ImGui if requested (rarely used, only when ImGuiConfigFlags_NavEnableSetMousePos is enabled by user)
        // (When multi-viewports are enabled, all Dear ImGui positions are same as OS positions)
        if (io.WantSetMousePos && focused) {
            win->set_cursor_pos((s32) (mousePosPrev.x - it->Pos.x), (s32) (mousePosPrev.y - it->Pos.y));
        }

        // Set Dear ImGui mouse position from OS position
        if (mouseWindow) {
            auto mousePos = win->get_cursor_pos();

            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
                // Multi-viewport mode: mouse position in OS absolute coordinates (io.MousePos is (0,0) when the mouse is on the upper-left of the primary monitor)
                auto winPos = win->get_pos();
                io.MousePos = ImVec2((f32) (mousePos.x + winPos.x), (f32) (mousePos.y + winPos.y));
            } else {
                // Single viewport mode: mouse position in client window coordinates (io.MousePos is (0,0) when the mouse is on the upper-left corner of the app window)
                io.MousePos = ImVec2((f32) mousePos.x, (f32) mousePos.y);
            }
        }

        bool windowNoInput = it->Flags & ImGuiViewportFlags_NoInputs;
        win->Flags         = set_bit(win->Flags, window::MOUSE_PASS_THROUGH, windowNoInput);
        if (win->is_hovered() && !windowNoInput) io.MouseHoveredViewport = it->ID;
    }

    if ((io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) || mainWindow->CursorMode == window::CURSOR_DISABLED)
        return;

    ImGuiMouseCursor imguiCursor = ImGui::GetMouseCursor();
    For(platformIO.Viewports) {
        window *win = (window *) it->PlatformHandle;
        if (imguiCursor == ImGuiMouseCursor_None || io.MouseDrawCursor) {
            // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
            win->set_cursor_mode(window::CURSOR_HIDDEN);
        } else {
            // Show OS mouse cursor
            win->set_cursor(&MouseCursors[imguiCursor]);
            win->set_cursor_mode(window::CURSOR_NORMAL);
        }
    }
}
