#include "pixel_buffer.h"

#include "lstd/memory/string.h"
#include "vendor/stb/stb_image.h"

import lstd.path;

LSTD_BEGIN_NAMESPACE

pixel_buffer::pixel_buffer(u8 *pixels, u32 width, u32 height, pixel_format format) {
    Pixels = pixels;
    Width = width;
    Height = height;
    Format = format;
    BPP = (s32) format;
}

pixel_buffer::pixel_buffer(const string &path, bool flipVertically, pixel_format format) {
    string pathNormalized = path_normalize(path);
    defer(free(pathNormalized));

    auto [content, success] = LSTD_NAMESPACE::path_read_entire_file(pathNormalized);
    if (!success) return;

    // stbi_set_flip_vertically_on_load(flipVertically);

    s32 w, h, n;
    u8 *loaded = stbi_load_from_memory(content.Data, (s32) content.Count, &w, &h, &n, (s32) format);

    Pixels = loaded;
    Width = w;
    Height = h;
    Format = (pixel_format) n;
    BPP = (s32) Format;
}

void pixel_buffer::release() {
    if (Reserved) free(Pixels);
    Pixels = null;
    Format = pixel_format::Unknown;
    Width = Height = BPP = 0;
}

pixel_buffer *clone(pixel_buffer *dest, pixel_buffer src) {
    *dest = src;
    s64 size = src.Width * src.Height * src.BPP;
    dest->Pixels = malloc<u8>({.Count = size});
    copy_memory(dest->Pixels, src.Pixels, size);
    return dest;
}

LSTD_END_NAMESPACE
