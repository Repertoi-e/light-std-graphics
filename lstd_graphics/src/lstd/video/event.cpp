#include "event.h"

LSTD_BEGIN_NAMESPACE

// Copyright 2019 Dietrich Epp.
// This file is licensed under the terms of the MIT license.
// https://github.com/depp/keycode

// These following tables have been automatically generated:
static const char *KEY_ID_DATA =
    "\0A\0B\0C\0CapsLock\0Comma\0D\0Delete\0DeleteForward\0E\0End\0Escape\0F\0"
    "F1\0F10\0F11\0F12\0F13\0F14\0F15\0F16\0F17\0F18\0F19\0F2\0F20\0F21\0F22\0"
    "F23\0F24\0F3\0F4\0F5\0F6\0F7\0F8\0F9\0G\0Grave\0H\0Help\0Home\0Insert\0J"
    "\0K\0KP0\0KP1\0KP2\0KP3\0KP4\0KP5\0KP6\0KP7\0KP8\0KP9\0KPAdd\0KPClear\0KP"
    "Decimal\0KPDivide\0KPEnter\0KPEquals\0KPMultiply\0KPNumLock\0KPPoint\0KPS"
    "ubtract\0L\0Left\0LeftAlt\0LeftBracket\0LeftControl\0LeftGUI\0LeftShift\0"
    "M\0Menu\0Minus\0Mute\0N\0NonUSBackslash\0O\0P\0PageDown\0PageUp\0Pause\0P"
    "eriod\0PrintScreen\0Q\0Quote\0R\0Return\0Right\0RightAlt\0RightBracket\0R"
    "ightControl\0RightGUI\0RightShift\0S\0ScrollLock\0Semicolon\0Slash\0Space"
    "\0SysReq\0T\0Tab\0U\0V\0W\0X\0Y\0Z";

static s16 KEY_ID_OFF[] = {
    0, 0, 0, 0, 1, 3, 5, 22, 45, 58, 147, 155, 353, 174, 176, 308, 365, 383, 400, 402, 445, 453,
    523, 565, 571, 573, 575, 577, 579, 581, 61, 73, 77, 81, 85, 89, 93, 97, 101, 65, 0, 51, 24, 567,
    552, 372, 261, 323, 477, 390, 0, 536, 447, 149, 16, 426, 546, 7, 60, 103, 126, 129, 132, 135, 138, 141,
    144, 63, 67, 71, 433, 525, 420, 167, 162, 413, 31, 47, 404, 462, 310, 408, 417, 279, 242, 268, 297, 218,
    251, 182, 186, 190, 194, 198, 202, 206, 210, 214, 178, 289, 385, 0, 0, 259, 75, 79, 83, 87, 91, 95,
    99, 106, 110, 114, 118, 122, 0, 157, 367, 0, 0, 0, 0, 0, 0, 0, 0, 378, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    558, 0, 0, 0, 455, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 224, 0, 0, 0,
    232, 0, 0, 0, 335, 355, 315, 347, 490, 512, 468, 503};

static s32 KEY_ID_ORDER[] = {39, 30, 31, 32, 33, 34, 35, 36, 37, 38, 4, 5, 49, 6, 57, 54, 7, 42,
                             76, 81, 8, 77, 46, 41, 9, 58, 67, 68, 69, 104, 105, 106, 107, 108, 109, 110,
                             59, 111, 112, 113, 114, 115, 60, 61, 62, 63, 64, 65, 66, 10, 53, 11, 117, 74,
                             12, 73, 13, 14, 98, 89, 90, 91, 92, 93, 94, 95, 96, 97, 87, 216, 220, 84,
                             88, 103, 85, 83, 99, 86, 15, 80, 226, 47, 224, 227, 225, 16, 118, 45, 127, 17,
                             100, 18, 19, 78, 75, 72, 55, 70, 20, 52, 21, 158, 79, 230, 48, 228, 231, 229,
                             22, 71, 51, 56, 44, 154, 23, 43, 24, 82, 25, 26, 27, 28, 29};

u32 key_code_from_name(const string &name) {
    char buffer[14];

    s32 n;
    for (n = 0; n < 14 && name.Length != n; ++n) {
        auto c = (char) name[n];
        if ('A' <= c && c <= 'Z') c |= 32;
        buffer[n] = c;
    }
    if (name.Length != n) return 0;

    // Binary search in sorted list of identifiers
    s32 left = 0, right = sizeof(KEY_ID_ORDER), center;
    while (left < right) {
        center = left + (right - left) / 2;
        u32 code = KEY_ID_ORDER[center];
        auto *p = KEY_ID_DATA + KEY_ID_OFF[code];
        for (int i = 0;; i++) {
            if (i >= n) {
                if (p[i] != '\0') {
                    right = center;
                    break;
                }
                return code;
            }
            s32 d = buffer[i] - ((char) p[i] | 32);
            if (d < 0) {
                right = center;
                break;
            } else if (d > 0) {
                left = center + 1;
                break;
            }
        }
    }
    return 0;
}

string key_name_from_code(u32 code) {
    if (code >= 232) return "";
    s16 off = KEY_ID_OFF[code];
    if (off == 0) return "";
    return string(KEY_ID_DATA + off);
}

LSTD_END_NAMESPACE
