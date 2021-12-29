#include "driver.h"

#if !defined BUILDING_DRIVER
#error Error
#endif

import lstd.os;
import lstd.fmt;
import lstd.path;

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
    assert(DLLFileName.Count);

    // This may be different from the working directory
    string exeDir = path_directory(os_get_current_module());

    DLLFile       = path_join(exeDir, DLLFileName);
    BuildLockFile = path_join(exeDir, "buildlock");
}

file_scope bool reload_code() {
    UpdateAndRender = null;
    MainWindowEvent = null;
    if (DLL) free_dynamic_library(DLL);

    string copyPath = path_join(path_directory(DLLFile), "loaded_code.dll");
    if (!path_copy(DLLFile, copyPath, true)) {
        print("Error: Couldn't write to {!YELLOW}\"{}\"{!}. Game already running?\n", copyPath);
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

void init_imgui_for_our_windows(window mainWindow);
void imgui_for_our_windows_new_frame();

// @TODO: Provide a library construct for parsing arguments automatically
file_scope void parse_arguments() {
    array<string> usage;
    make_dynamic(&usage, 8);

    add(&usage, string("Usage:\n"));
    add(&usage,
        string("    {!YELLOW}-dll <name>{!GRAY}    "
               "Specifies which dll to hot load in the engine. By default it searches for cars.dll{!}\n\n"));
    add(&usage,
        string("    {!YELLOW}-memory <amount>{!GRAY}    "
               "Specifies the amount of memory (in MiB) which gets reserved for the app (default is 128 MiB)."
               "Usage must never cross this.{!}\n\n"));
    add(&usage,
        string("    {!YELLOW}-width <value>{!GRAY}    "
               "Specifies the width of the app window (default is 1200).{!}\n\n"));
    add(&usage,
        string("    {!YELLOW}-heigth <value>{!GRAY}    "
               "Specifies the height of the app window (default is 600).\n\n"));
    add(&usage,
        string("    {!YELLOW}-fps <value>{!GRAY}    "
               "Specifies the target fps (default is 60).\n\n"));
    defer(free(usage.Data));

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
                print(">>> {!RED}Couldn't parse memory value. The argument was: \"{}\"\n", it);
            } else if (status == PARSE_TOO_MANY_DIGITS) {
                print(">>> {!RED}Memory value too big. The argument was: \"{}\"\n", it);
            } else {
                MemoryInBytes = (s64) value * 1_MiB;
            }
            seekMemory = false;
            continue;
        }
        if (seekWidth) {
            auto [value, status, rest] = parse_int<u32, parse_int_options{.ParseSign = false, .LookForBasePrefix = true}>(it, 10);

            if (status == PARSE_INVALID) {
                print(">>> {!RED}Couldn't parse width value. The argument was: \"{}\"\n", it);
            } else if (status == PARSE_TOO_MANY_DIGITS) {
                print(">>> {!RED}Width value too big. The argument was: \"{}\"\n", it);
            } else {
                WindowWidth = value;
            }
            seekWidth = false;
            continue;
        }
        if (seekHeight) {
            auto [value, status, rest] = parse_int<u32, parse_int_options{.ParseSign = false, .LookForBasePrefix = true}>(it, 10);

            if (status == PARSE_INVALID) {
                print(">>> {!RED}Couldn't parse height value. The argument was: \"{}\"\n", it);
            } else if (status == PARSE_TOO_MANY_DIGITS) {
                print(">>> {!RED}Height value too big. The argument was: \"{}\"\n", it);
            } else {
                WindowHeight = value;
            }
            seekHeight = false;
            continue;
        }
        if (seekFPS) {
            auto [value, status, rest] = parse_int<u32, parse_int_options{.ParseSign = false, .LookForBasePrefix = true}>(it, 10);

            if (status == PARSE_INVALID) {
                print(">>> {!RED}Couldn't parse FPS value. The argument was: \"{}\"\n", it);
            } else if (status == PARSE_TOO_MANY_DIGITS) {
                print(">>> {!RED}FPS value too big. The argument was: \"{}\"\n", it);
            } else {
                TargetFPS = value;
            }
            seekFPS = false;
            continue;
        }
        if (strings_match(it, "-dll")) {
            seekFileName = true;
        } else if (strings_match(it, "-memory")) {
            seekMemory = true;
        } else if (strings_match(it, "-width")) {
            seekWidth = true;
        } else if (strings_match(it, "-height")) {
            seekHeight = true;
        } else if (strings_match(it, "-fps")) {
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
    auto pack = os_allocate_packed<tlsf_allocator_data, memory, graphics>(MemoryInBytes);

    auto data = tuple_get<0>(pack);
    auto m    = tuple_get<1>(pack);
    auto g    = tuple_get<2>(pack);
    auto pool = tuple_get<3>(pack);

    PersistentAlloc = {tlsf_allocator, data};
    tlsf_allocator_add_pool(data, pool, MemoryInBytes);

    Memory   = m;
    Graphics = g;

    auto newContext  = Context;
    newContext.Log   = &cout;
    newContext.Alloc = PersistentAlloc;
    // newContext.LogAllAllocations = true;
    OVERRIDE_CONTEXT(newContext);

    m->PersistentAlloc = PersistentAlloc;

    TemporaryAllocatorData.Block = os_allocate_block(1_MiB);
    TemporaryAllocatorData.Size  = 1_MiB;

    m->TemporaryAlloc = TemporaryAllocator;

    parse_arguments();
    setup_paths();

    string windowTitle = "Socraft";

    auto windowFlags = window::SHOWN | window::RESIZABLE | window::VSYNC | window::FOCUS_ON_SHOW | window::CLOSE_ON_ALT_F4 | window::SCALE_TO_MONITOR;
    m->MainWindow    = create_window(windowTitle, window::DONT_CARE, window::DONT_CARE, WindowWidth, WindowHeight, windowFlags);

    auto icon = make_bitmap("data/icon.png", false, pixel_format::RGBA);

    array<bitmap> icons;
    make_dynamic(&icons, 1);

    add(&icons, icon);
    m->MainWindow.set_icon(icons);

    free(icon.Pixels);
    free(icons.Data);

    m->MainWindow.connect_event(delegate<bool(event e)>([](auto e) {
        if (MainWindowEvent) return MainWindowEvent(e);
        return false;
    }));

    m->GetStateImpl = [](string name, s64 size, bool *created) {
        string identifier = name;
        make_dynamic(&identifier, 5 + name.Count);

        string_append(&identifier, "Ident");

        auto **found = find(&Memory->States, identifier).Value;
        if (!found) {
            // We allocate with alignment 16 for SIMD
            void *result = malloc<char>({.Count = size, .Alignment = 16});
            add(&Memory->States, identifier, result);
            *created = true;
            return result;
        }

        free(identifier.Data);
        return *found;
    };

    // @TODO: TargetFPS currently does nothing, we rely on g.swap() and vsync to hit target monitor refresh rate

    // This is affecting any physics time steps though
    m->FrameDelta = 1.0f / TargetFPS;

    g->init(graphics_api::Direct3D);
    g->set_blend(true);
    g->set_depth_testing(false);

    m->ImGuiMemAlloc = [](size_t size, void *) { return (void *) malloc<char>({.Count = (s64) size, .Alloc = PersistentAlloc}); };
    m->ImGuiMemFree  = [](void *ptr, void *) { free(ptr); };
    ImGui::SetAllocatorFunctions(m->ImGuiMemAlloc, m->ImGuiMemFree);

    ImGui::CreateContext();
    m->ImGuiContext = ImGui::GetCurrentContext();
    defer(ImGui::DestroyContext());  // To save the .ini file

    init_imgui_for_our_windows(m->MainWindow);

    imgui_renderer imguiRenderer;
    imguiRenderer.init(g);
    defer(imguiRenderer.release());

    while (true) {
        m->ReloadedThisFrame = check_for_dll_change();
        if (m->RequestReloadNextFrame) {
            m->ReloadedThisFrame      = reload_code();
            m->RequestReloadNextFrame = false;
        }

        update_windows();
        if (m->MainWindow.is_destroying()) break;

        imgui_for_our_windows_new_frame();
        ImGui::NewFrame();
        if (UpdateAndRender) UpdateAndRender(Memory, Graphics);
        ImGui::Render();

        if (m->MainWindow.is_visible()) {
            g->set_target_window(m->MainWindow);
            g->set_cull_mode(cull::None);
            imguiRenderer.draw(ImGui::GetDrawData());
            g->swap();
        }
    }
}

struct imgui_our_windows_data {
    window Window;
    window MouseWindow;

    s64 Time = 0;

    // So we don't miss click-release events that are shorter than 1 frame...
    // This gets zeroed not when the mouse is released but in _imgui_for_our_windows_new_frame()_.
    bool MouseJustPressed[Mouse_Button_Last + 1]{};

    bool MouseButtons[Mouse_Button_Last + 1]{};
    cursor *MouseCursors[ImGuiMouseCursor_COUNT];

    window KeyOwnerWindows[Key_Last + 1]{};

    bool InstalledCallbacks = false;
};

// This can be global

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

file_scope bool common_event_callback(event e) {
    ImGuiIO &io = ImGui::GetIO();
    auto *bd    = (imgui_our_windows_data *) io.BackendPlatformUserData;

    if (e.Type == event::Key_Pressed) {
        io.KeysDown[e.KeyCode]         = true;
        bd->KeyOwnerWindows[e.KeyCode] = e.Window;
        update_modifiers();
    } else if (e.Type == event::Key_Released) {
        io.KeysDown[e.KeyCode]         = false;
        bd->KeyOwnerWindows[e.KeyCode] = {};
        update_modifiers();
    } else if (e.Type == event::Code_Point_Typed) {
        io.AddInputCharacter(e.CP);
    } else if (e.Type == event::Mouse_Button_Pressed) {
        bd->MouseJustPressed[e.Button] = true;
        bd->MouseButtons[e.Button]     = true;
    } else if (e.Type == event::Mouse_Button_Released) {
        bd->MouseButtons[e.Button] = false;
    } else if (e.Type == event::Mouse_Wheel_Scrolled) {
        io.MouseWheelH += e.ScrollX;
        io.MouseWheel += e.ScrollY;
    } else if (e.Type == event::Window_Focused) {
        io.AddFocusEvent(e.Focused);
    } else if (e.Type == event::Mouse_Entered_Window) {
        bd->MouseWindow = e.Window;
    } else if (e.Type == event::Mouse_Left_Window) {
        bd->MouseWindow = {};
    }

    return false;
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

file_scope void init_imgui_for_our_windows(window mainWindow) {
    ImGuiIO &io = ImGui::GetIO();
    io.Fonts    = malloc<ImFontAtlas>();

    auto *bd                   = malloc<imgui_our_windows_data>();
    io.BackendPlatformUserData = (void *) bd;
    io.BackendPlatformName     = "imgui_impl_lstd_graphics";
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;  // We can honor GetMouseCursor() values (optional)
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;   // We can honor io.WantSetMousePos requests (optional, rarely used)
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    bd->Window = mainWindow;

    // We tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle &style = ImGui::GetStyle();

    ImGui::StyleColorsDark();
    imgui_init_photoshop_style();

    io.SetClipboardTextFn = [](auto, const char *text) { os_set_clipboard_content(string(text)); };
    io.GetClipboardTextFn = [](auto) {
        static array<char> buffer;

        // ImGui expects a null-terminated string.
        // @TODO @Speed Make os_get_clipboard_content() ensure it is null terminated to avoid allocating. Similar to os_get_env.

        auto content = os_get_clipboard_content();
        resize(&buffer, content.Count + 1);

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
    io.KeyMap[ImGuiKey_Delete]      = Key_DeleteForward;
    io.KeyMap[ImGuiKey_Backspace]   = Key_Delete;
    io.KeyMap[ImGuiKey_Space]       = Key_Space;
    io.KeyMap[ImGuiKey_Enter]       = Key_Return;
    io.KeyMap[ImGuiKey_Escape]      = Key_Escape;
    io.KeyMap[ImGuiKey_KeyPadEnter] = KP_Enter;
    io.KeyMap[ImGuiKey_A]           = Key_A;
    io.KeyMap[ImGuiKey_C]           = Key_C;
    io.KeyMap[ImGuiKey_V]           = Key_V;
    io.KeyMap[ImGuiKey_X]           = Key_X;
    io.KeyMap[ImGuiKey_Y]           = Key_Y;
    io.KeyMap[ImGuiKey_Z]           = Key_Z;

    bd->InstalledCallbacks = true;
    mainWindow.connect_event(common_event_callback);

    bd->MouseCursors[0] = make_cursor(OS_ARROW);
    bd->MouseCursors[1] = make_cursor(OS_IBEAM);
    bd->MouseCursors[2] = make_cursor(OS_RESIZE_ALL);
    bd->MouseCursors[3] = make_cursor(OS_RESIZE_NS);
    bd->MouseCursors[4] = make_cursor(OS_RESIZE_WE);
    bd->MouseCursors[5] = make_cursor(OS_RESIZE_NESW);
    bd->MouseCursors[6] = make_cursor(OS_RESIZE_NWSE);
    bd->MouseCursors[7] = make_cursor(OS_HAND);
}

file_scope u32 set_bit(u32 number, u32 mask, bool enabled) {
    return number & ~mask | mask & (enabled ? 0xFFFFFFFF : 0);
}

file_scope void imgui_for_our_windows_new_frame() {
    auto *bd = (imgui_our_windows_data *) ImGui::GetIO().BackendPlatformUserData;

    ImGuiIO &io = ImGui::GetIO();
    assert(io.Fonts->IsBuilt());

    auto mainWindow = bd->Window;

    int2 windowSize = mainWindow.get_size();
    s32 w = windowSize.x, h = windowSize.y;

    int2 frameBufferSize = mainWindow.get_framebuffer_size();
    io.DisplaySize       = ImVec2((f32) w, (f32) h);
    if (w > 0 && h > 0) io.DisplayFramebufferScale = ImVec2((f32) frameBufferSize.x / w, (f32) frameBufferSize.y / h);

    time_t now   = os_get_time();
    io.DeltaTime = bd->Time > 0 ? (f32) os_time_to_seconds(now - bd->Time) : (1.0f / 60.0f);
    bd->Time     = now;

    ImVec2 mousePosPrev = io.MousePos;
    io.MousePos         = ImVec2(-FLT_MAX, -FLT_MAX);

    // Update mouse buttons.
    // If a mouse press event came, always pass it as "mouse held this frame",
    // so we don't miss click-release events that are shorter than 1 frame.
    // That's why we have the _MouseJustPressed_ array.
    For(range(Mouse_Button_Last + 1)) {
        io.MouseDown[it] = bd->MouseJustPressed[it] || bd->MouseButtons[it];
    }
    zero_memory(bd->MouseJustPressed, sizeof(bd->MouseJustPressed));

    auto win = bd->Window;

    bool focused       = win.get_flags() & window::FOCUSED;
    window mouseWindow = (bd->MouseWindow == win || focused) ? win : window{};

    // Set OS mouse position from Dear ImGui if requested (rarely used, only when
    // ImGuiConfigFlags_NavEnableSetMousePos is enabled by user).
    if (io.WantSetMousePos && focused) {
        win.set_cursor_pos((s32) mousePosPrev.x, (s32) mousePosPrev.y);
    }

    // Set Dear ImGui mouse position from OS position
    if (mouseWindow) {
        auto mousePos = win.get_cursor_pos();
        io.MousePos   = ImVec2((f32) mousePos.x, (f32) mousePos.y);
    }

    if ((io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) || mainWindow.get_cursor_mode() == window::CURSOR_DISABLED)
        return;

    ImGuiMouseCursor imguiCursor = ImGui::GetMouseCursor();
    if (imguiCursor == ImGuiMouseCursor_None || io.MouseDrawCursor) {
        // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
        win.set_cursor_mode(window::CURSOR_HIDDEN);
    } else {
        // Show OS mouse cursor
        win.set_cursor(bd->MouseCursors[imguiCursor]);
        win.set_cursor_mode(window::CURSOR_NORMAL);
    }
}
