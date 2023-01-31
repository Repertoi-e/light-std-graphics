#pragma once

#include "../graphics/bitmap.h"

enum os_cursor : u32 {
    OS_APPSTARTING,  // Standard arrow and small hourglass

    OS_ARROW,
    OS_IBEAM,
    OS_CROSSHAIR,
    OS_HAND,

    OS_HELP,  // Arrow and question mark
    OS_NO,    // Slashed circle

    OS_RESIZE_ALL,
    OS_RESIZE_NESW,
    OS_RESIZE_NS,
    OS_RESIZE_NWSE,
    OS_RESIZE_WE,

    OS_UP_ARROW,  // Vertical arrow
    OS_WAIT       // Hourglass
};

struct cursor {
    union platform_data {
        struct {
            void *hCursor      = null;
            bool ShouldDestroy = false;
        } Win32;
    } PlatformData{};

    // We keep track of created cursors in a linked list
    cursor *Next = null;
};

cursor *make_cursor(bitmap *image, int2 hotSpot);
cursor *make_cursor(os_cursor osCursor);

void free_cursor(cursor *c);
