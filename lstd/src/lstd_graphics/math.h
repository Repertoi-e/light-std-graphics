#pragma once

#include <lstd/common.h>

struct v2 {
    f32 x, y;
};

struct v3 {
    f32 x, y, z;
};

struct v4 {
    union {
        struct {
            f32 x, y, z, w;
        };
        struct {
            f32 r, g, b, a;
        };
    };
};

struct v2i {
    s32 x, y;
};

struct rect {
    s32 top, left;
    s32 bottom, right;
};