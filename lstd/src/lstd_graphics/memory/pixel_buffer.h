#pragma once

LSTD_BEGIN_NAMESPACE

enum class pixel_format : s32 { Unknown = 0,
                                Grey = 1,
                                Grey_Alpha = 2,
                                RGB = 3,
                                RGBA = 4 };

struct pixel_buffer {
    pixel_format Format = pixel_format::Unknown;
    u32 Width = 0, Height = 0;
    s32 BPP = (s32) Format;  // BPP is equal to (s32) Format
    u8 *Pixels = null;
    s64 Reserved = 0;

    pixel_buffer() {}

    // Just points to buffer (may get invalidated)
    pixel_buffer(u8 *pixels, u32 width, u32 height, pixel_format format);

    //
    // Loads from a file.
    //
    // If _format_ is not passed as _Unknown_, the file is loaded and converted to the requested one.
    // The _Format_ member is set at _Unknown_ if the load failed.
    pixel_buffer(string path, bool flipVertically = false, pixel_format format = pixel_format::Unknown);

    void release();
};

pixel_buffer *clone(pixel_buffer *dest, pixel_buffer src);

LSTD_END_NAMESPACE
