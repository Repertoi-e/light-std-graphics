#pragma once

#include "../math.h"

import "lstd.h";

enum class pixel_format : s32 { Unknown    = 0,
                                Grey       = 1,
                                Grey_Alpha = 2,
                                RGB        = 3,
                                RGBA       = 4 };

struct bitmap {
    pixel_format Format = pixel_format::Unknown;

    u32 Width = 0, Height = 0;
    s32 BPP    = (s32) Format;  // BPP is bytes per pixel
    u8 *Pixels = null;

    s64 Allocated = 0;
};

// Points to a buffer
bitmap make_bitmap(u8 *pixels, u32 width, u32 height, pixel_format format);

//
// Loads from a file.
//
// If _format_ is not passed as _Unknown_, the file is loaded and converted to the requested one.
// The _Format_ member is set at _Unknown_ if the load failed.
bitmap make_bitmap(string path, bool flipVertically = false, pixel_format format = pixel_format::Unknown);

void free_bitmap(bitmap *b);

bitmap clone(bitmap *src);
