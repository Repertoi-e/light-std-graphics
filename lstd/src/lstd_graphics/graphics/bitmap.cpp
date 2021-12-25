#include "bitmap.h"

#include "vendor/stb/stb_image.h"

import lstd.path;

bitmap make_bitmap(u8 *pixels, u32 width, u32 height, pixel_format format) {
    bitmap r;

    r.Pixels = pixels;
    r.Width  = width;
    r.Height = height;
    r.Format = format;
    r.BPP    = (s32) format;

    return r;
}

bitmap make_bitmap(string p, bool flipVertically, pixel_format format) {
    string path = path_normalize(p);
    defer(free(path.Data));

    auto [content, success] = os_read_entire_file(path);
    if (!success) return;

    // stbi_set_flip_vertically_on_load(flipVertically);

    s32 w, h, n;
    u8 *loaded = stbi_load_from_memory(content.Data, (s32) content.Count, &w, &h, &n, (s32) format);

    Pixels = loaded;
    Width  = w;
    Height = h;
    Format = (pixel_format) n;
    BPP    = (s32) Format;
}

void free_bitmap(bitmap *b) {
    if (b->Allocated) free(b->Pixels);
    b->Pixels = null;
    b->Format = pixel_format::Unknown;
    b->Width  = 0;
    b->Height = 0;
    b->BPP    = 0;
}

bitmap clone(bitmap *src) {
    bitmap b;
    b = *src;

    s64 size = src->Width * src->Height * src->BPP;

    b.Pixels = malloc<u8>({.Count = size});
    copy_memory(b.Pixels, src->Pixels, size);

    return b;
}
