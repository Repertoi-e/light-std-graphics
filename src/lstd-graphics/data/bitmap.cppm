module;

#include "lstd-graphics/third_party/stb/stb_image.h"

export module g.bitmap;

import "lstd.h";

import lstd.path;
import lstd.os;

export {
	enum class pixel_format : s32 {
		UNKNOWN = 0,
		GREY = 1,
		GREY_ALPHA = 2,
		RGB = 3,
		RGBA = 4
	};

	struct bitmap {
		pixel_format Format = pixel_format::UNKNOWN;

		u32 Width = 0, Height = 0;
		s32 BPP = (s32)Format;  // BPP is bytes per pixel
		u8* Pixels = null;

		s64 Allocated = 0;
	};

	// Points to a buffer
	bitmap make_bitmap(u8* pixels, u32 width, u32 height, pixel_format format);

	//
	// Loads from a file.
	//
	// If _format_ is not passed as _UNKNOWN_, the file is loaded 
	// and converted to the requested one. The _Format_ member is 
	// set at _UNKNOWN_ if the load failed.
	bitmap make_bitmap(string path, bool flipVertically = false, pixel_format format = pixel_format::UNKNOWN);

	void free(bitmap* b);
	bitmap clone(bitmap* src);
}

bitmap make_bitmap(u8* pixels, u32 width, u32 height, pixel_format format) {
	bitmap r;

	r.Pixels = pixels;
	r.Width = width;
	r.Height = height;
	r.Format = format;
	r.BPP = (s32)format;

	return r;
}

bitmap make_bitmap(string p, bool flipVertically, pixel_format format) {
	string path = path_normalize(p);
	defer(free(path.Data));

	auto [content, success] = os_read_entire_file(path);
	if (!success) return {};

	// stbi_set_flip_vertically_on_load(flipVertically);

	s32 w, h, n;
	u8* loaded = stbi_load_from_memory((u8*)content.Data, (s32)content.Count, &w, &h, &n, (s32)format);

	bitmap b;

	b.Pixels = loaded;
	b.Width = w;
	b.Height = h;
	b.Format = (pixel_format)n;
	b.BPP = (s32)b.Format;
	return b;
}

void free_bitmap(bitmap* b) {
	if (b->Allocated) free(b->Pixels);
	b->Pixels = null;
	b->Format = pixel_format::UNKNOWN;
	b->Width = 0;
	b->Height = 0;
	b->BPP = 0;
}

bitmap clone(bitmap* src) {
	bitmap b;
	b = *src;

	s64 size = src->Width * src->Height * src->BPP;

	b.Pixels = malloc<u8>({ .Count = size });
	memcpy(b.Pixels, src->Pixels, size);

	return b;
}


