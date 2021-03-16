//-----------------------------------------------------------------------------
// COMPILE-TIME OPTIONS FOR DEAR IMGUI
// Runtime options (clipboard callbacks, enabling various features, etc.) can generally be set via the ImGuiIO
// structure. You can use ImGui::SetAllocatorFunctions() before calling ImGui::CreateContext() to rewire memory
// allocation functions.
//-----------------------------------------------------------------------------
// A) You may edit imconfig.h (and not overwrite it when updating Dear ImGui, or maintain a patch/branch with your
// modifications to imconfig.h) B) or add configuration directives in your own file and compile with #define
// IMGUI_USER_CONFIG "myfilename.h" If you do so you need to make sure that configuration settings are defined
// consistently _everywhere_ Dear ImGui is used, which include the imgui*.cpp files but also _any_ of your code that
// uses Dear ImGui. This is because some compile-time options have an affect on data structures. Defining those options
// in imconfig.h will ensure every compilation unit gets to see the same data structure layouts. Call
// IMGUI_CHECKVERSION() from your .cpp files to verify that the data structures your files are using are matching the
// ones imgui.cpp is using.
//-----------------------------------------------------------------------------

#pragma once

#include "lstd/math/vec.h"
#include "lstd/memory/string_utils.h"
#include "vendor/linasm/include/Array.h"
#include "vendor/linasm/include/Numbers.h"
#include "vendor/linasm/include/String.h"

//---- Define assertion handler. Defaults to calling assert().
#define IM_ASSERT(_EXPR) assert(_EXPR)

//---- Define attributes of all API symbols declarations, e.g. for DLL under Windows
// Using dear imgui via a shared library is not recommended, because of function call overhead and because we don't
// guarantee backward nor forward ABI compatibility.
//#define IMGUI_API __declspec( dllexport )
//#define IMGUI_API __declspec( dllimport )

//---- Don't define obsolete functions/enums names. Consider enabling from time to time after updating to avoid using
// soon-to-be obsolete function/names.
#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS

//---- Don't implement demo windows functionality (ShowDemoWindow()/ShowStyleEditor()/ShowUserGuide() methods will be
// empty)
// It is very strongly recommended to NOT disable the demo windows during development. Please read the comments in
// imgui_demo.cpp.
//#define IMGUI_DISABLE_DEMO_WINDOWS
//#define IMGUI_DISABLE_METRICS_WINDOW

//---- Don't implement some functions to reduce linkage requirements.
// #define IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS
// [Win32] Don't implement default clipboard handler. Won't use and link with
// OpenClipboard/GetClipboardData/CloseClipboard etc.
//
// #define IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS
// [Win32] Don't implement default IME handler. Won't use and link with ImmGetContext/ImmSetCompositionWindow.
//
// #define IMGUI_DISABLE_WIN32_FUNCTIONS
// [Win32] Won't use and link with any Win32 function (clipboard, ime).
//
// #define IMGUI_ENABLE_OSX_DEFAULT_CLIPBOARD_FUNCTIONS
// [OSX] Implement default OSX clipboard handler (need to link with '-framework ApplicationServices')
//
#define IMGUI_DISABLE_FORMAT_STRING_FUNCTIONS
// Don't implement ImFormatString/ImFormatStringV so you can implement them yourself if you don't want to link with
// vsnprintf.
//
#define IMGUI_DISABLE_MATH_FUNCTIONS
// Don't implement ImFabs/ImSqrt/ImPow/ImFmod/ImCos/ImSin/ImAcos/ImAtan2 wrapper so you can implement them yourself.
// Declare your prototypes in imconfig.h.
//

#define IMGUI_DISABLE_DEFAULT_ALLOCATORS
// Don't implement default allocators calling malloc()/free() to avoid linking with them.
// You will need to call ImGui::SetAllocatorFunctions().

#define IMGUI_USE_STB_SPRINTF

//---- Include imgui_user.h at the end of imgui.h as a convenience
//#define IMGUI_INCLUDE_IMGUI_USER_H

//---- Pack colors to BGRA8 instead of RGBA8 (to avoid converting from one to another)
//#define IMGUI_USE_BGRA_PACKED_COLOR

//---- Avoid multiple STB libraries implementations, or redefine path/filenames to prioritize another version
// By default the embedded implementations are declared static and not available outside of imgui cpp files.
#define IMGUI_STB_TRUETYPE_FILENAME "../stb/stb_truetype.h"
#define IMGUI_STB_RECT_PACK_FILENAME "../stb/stb_rect_pack.h"
// #define IMGUI_DISABLE_STB_TRUETYPE_IMPLEMENTATION
// #define IMGUI_DISABLE_STB_RECT_PACK_IMPLEMENTATION

//---- Define constructor and implicit cast operators to convert back<>forth between your math types and ImVec2/ImVec4.
// This will be inlined as part of ImVec2 and ImVec4 class declarations.
#define IM_VEC2_CLASS_EXTRA     \
    ImVec2(const lstd::v2 &f) { \
        x = f.x;                \
        y = f.y;                \
    }                           \
    operator LSTD_NAMESPACE::v2() const { return LSTD_NAMESPACE::v2(x, y); }

#define IM_VEC4_CLASS_EXTRA     \
    ImVec4(const lstd::v4 &f) { \
        x = f.x;                \
        y = f.y;                \
        z = f.z;                \
        w = f.w;                \
    }                           \
    operator LSTD_NAMESPACE::v4() const { return LSTD_NAMESPACE::v4(x, y, z, w); }

//---- Using 32-bits vertex indices (default is 16-bits) is one way to allow large meshes with more than 64K vertices.
// Your renderer back-end will need to support it (most example renderer back-ends support both 16/32-bits indices).
// Another way to allow large meshes while keeping 16-bits indices is to handle ImDrawCmd::VtxOffset in your renderer.
// Read about ImGuiBackendFlags_RendererHasVtxOffset for details.
#define ImDrawIdx u32

//---- Override ImDrawCallback signature (will need to modify renderer back-ends accordingly)
// struct ImDrawList;
// struct ImDrawCmd;
// typedef void (*MyImDrawCallback)(const ImDrawList* draw_list, const ImDrawCmd* cmd, void* my_renderer_user_data);
//#define ImDrawCallback MyImDrawCallback

//---- Debug Tools
// Use 'Metrics->Tools->Item Picker' to pick widgets with the mouse and break into them for easy debugging.
//#define IM_DEBUG_BREAK  IM_ASSERT(0)
//#define IM_DEBUG_BREAK  __debugbreak()
// Have the Item Picker break in the ItemAdd() function instead of ItemHoverable() - which is earlier in the code, will
// catch a few extra items, allow picking items other than Hovered one. This adds a small runtime cost which is why it
// is not enabled by default.
//#define IMGUI_DEBUG_TOOL_ITEM_PICKER_EX

//---- Tip: You can add extra functions within the ImGui:: namespace, here or in your own headers files.
/*
namespace ImGui {

}  // namespace ImGui
*/

#define memset LSTD_NAMESPACE::fill_memory
#define memcpy LSTD_NAMESPACE::copy_memory
#define memmove LSTD_NAMESPACE::copy_memory

#define strcmp String_Compare_char8
#define memcmp Array_Compare

#define ImSqrt Math_Sqrt_flt32
#define ImFabs Math_Abs_flt32
#define ImPow Math_ExpB_flt32

// https://git.musl-libc.org/cgit/musl/tree/src/math/fmod.c
static inline double ImFmod(double x, double y) {
    union {
        double f;
        u64 i;
    } ux = {x}, uy = {y};
    int ex = ux.i >> 52 & 0x7ff;
    int ey = uy.i >> 52 & 0x7ff;
    int sx = ux.i >> 63;
    u64 i;

    /* in the followings uxi should be ux.i, but then gcc wrongly adds */
    /* float load/store to inner loops ruining performance and code size */
    u64 uxi = ux.i;

    if (uy.i << 1 == 0 || LSTD_NAMESPACE::is_nan(y) || ex == 0x7ff)
        return (x * y) / (x * y);
    if (uxi << 1 <= uy.i << 1) {
        if (uxi << 1 == uy.i << 1)
            return 0 * x;
        return x;
    }

    /* normalize x and y */
    if (!ex) {
        for (i = uxi << 12; i >> 63 == 0; ex--, i <<= 1)
            ;
        uxi <<= -ex + 1;
    } else {
        uxi &= (u64)(-1) >> 12;
        uxi |= 1ULL << 52;
    }
    if (!ey) {
        for (i = uy.i << 12; i >> 63 == 0; ey--, i <<= 1)
            ;
        uy.i <<= -ey + 1;
    } else {
        uy.i &= (u64)(-1) >> 12;
        uy.i |= 1ULL << 52;
    }

    /* x mod y */
    for (; ex > ey; ex--) {
        i = uxi - uy.i;
        if (i >> 63 == 0) {
            if (i == 0)
                return 0 * x;
            uxi = i;
        }
        uxi <<= 1;
    }
    i = uxi - uy.i;
    if (i >> 63 == 0) {
        if (i == 0)
            return 0 * x;
        uxi = i;
    }
    for (; uxi >> 52 == 0; uxi <<= 1, ex--)
        ;

    /* scale result */
    if (ex > 0) {
        uxi -= 1ULL << 52;
        uxi |= (u64) ex << 52;
    } else {
        uxi >>= -ex + 1;
    }
    uxi |= (u64) sx << 63;
    ux.i = uxi;
    return ux.f;
}
static inline float ImFmod(float x, float y) { return (float) ImFmod((double) x, (double) y); }

#define ImCos Math_Cos_flt32
#define ImSin Math_Sin_flt32
#define ImAcos Math_ArcCos_flt32
#define ImAtan2 Math_ArcTan2_flt32
#define ImFloorStd Math_RoundDown_flt32
#define ImCeil Math_RoundUp_flt32

static inline double ImAtof(const char *s) {
    double ret;
    u32 status = Numbers_DecToNum_flt64(&ret, s);
    if (status == -1 && status == 0) return 0;
    return ret;
}

#define strlen String_Len_char8

#define strncmp String_CompareN_char8

inline const char *strstr(const char *n, const char *s) {
    u32 r = String_FindSubStringFwd_char8(n, s);
    if (r == -1) return null;
    return n + r;
}

inline const char *strchr(const char *n, char ch) {
    u32 r = String_FindSymbolFwd_char8(n, ch);
    if (r == -1) return null;
    return n + r;
}

inline char *strcpy(char *destination, const char *source) {
    if (!destination) return null;
    char *ptr = destination;

    while (*source != '\0') {
        *destination = *source;
        destination++;
        source++;
    }
    *destination = '\0';
    return ptr;
}

inline const char *memchr(const char *s, char ch, u64 textSize) {
    u32 r = Array_FindFwd_sint8(s, (u32) textSize, ch);
    if (r == -1) return null;
    return s + r;
}

#define FLT_MAX F32_MAX
#define FLT_MIN F32_MIN

#define DBL_MAX F64_MAX
#define DBL_MIN F64_MIN
