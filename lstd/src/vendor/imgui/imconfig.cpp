#include "imconfig.h"

extern "C" {

void *memset(void *ptr, int value, size_t n) { return LSTD_NAMESPACE::fill_memory(ptr, value, n); }
void *memcpy(void *dest, const void *src, size_t n) { return LSTD_NAMESPACE::copy_memory(dest, src, n); }
void *memmove(void *dest, const void *src, size_t n) { return LSTD_NAMESPACE::copy_memory(dest, src, n); }
u64 strlen(const char *str) { return LSTD_NAMESPACE::c_string_length(str); }

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) s1++, s2++;
    return *(const unsigned char *) s1 - *(const unsigned char *) s2;
}

int memcmp(const void *ptr1, const void *ptr2, u64 n) {
    auto *s1 = (const char *) ptr1;
    auto *s2 = (const char *) ptr2;
    while (n-- > 0) {
        if (*s1++ != *s2++) return s1[-1] < s2[-1] ? -1 : 1;
    }
    return 0;
}

const char *strstr(const char *ss1, const char *ss2) {
    auto s1 = LSTD_NAMESPACE::string(ss1);
    auto s2 = LSTD_NAMESPACE::string(ss2);
    auto r = LSTD_NAMESPACE::find_substring(s1, s2);
    if (r == -1) return null;
    return ss1 + r;
}

const char *strchr(const char *str, int character) {
    auto s = LSTD_NAMESPACE::string(str);
    auto r = LSTD_NAMESPACE::find_cp(s, character);
    if (r == -1) return null;
    return str + r;
}

char *strcpy(char *destination, const char *source) {
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

const void *memchr(const void *ptr, int value, u64 n) {
    const unsigned char *src = (const unsigned char *) ptr;
    while (n-- > 0) {
        if (*src == value)
            return (void *) src;
        src++;
    }
    return NULL;
}

//
// fmod source code taken from glibc
//

typedef union {
    double value;
    struct
    {
        u32 msw;
        u32 lsw;
    } parts;
    u64 word;
} ieee_double_shape_type;

/* Get two 32 bit ints from a double.  */
#define EXTRACT_WORDS(ix0, ix1, d)   \
    do {                             \
        ieee_double_shape_type ew_u; \
        ew_u.value = (d);            \
        (ix0) = ew_u.parts.msw;      \
        (ix1) = ew_u.parts.lsw;      \
    } while (0)

/* Set a double from two 32 bit ints.  */
#define INSERT_WORDS(d, ix0, ix1)    \
    do {                             \
        ieee_double_shape_type iw_u; \
        iw_u.parts.msw = (ix0);      \
        iw_u.parts.lsw = (ix1);      \
        (d) = iw_u.value;            \
    } while (0)

static const double one = 1.0, Zero[] = {0.0, -0.0};

double fmod(double x, double y) {
    s32 n, hx, hy, hz, ix, iy, sx, i;
    u32 lx, ly, lz;

    EXTRACT_WORDS(hx, lx, x);
    EXTRACT_WORDS(hy, ly, y);
    sx = hx & 0x80000000; /* sign of x */
    hx ^= sx;             /* |x| */
    hy &= 0x7fffffff;     /* |y| */

    /* purge off exception values */
    if ((hy | ly) == 0 || (hx >= 0x7ff00000) ||   /* y=0,or x not finite */
        ((hy | ((ly | -ly) >> 31)) > 0x7ff00000)) /* or y is NaN */
        return (x * y) / (x * y);
    if (hx <= hy) {
        if ((hx < hy) || (lx < ly)) return x; /* |x|<|y| return x */
        if (lx == ly)
            return Zero[(u32) sx >> 31]; /* |x|=|y| return x*0*/
    }

    /* determine ix = ilogb(x) */
    if (hx < 0x00100000) { /* subnormal x */
        if (hx == 0) {
            for (ix = -1043, i = lx; i > 0; i <<= 1) ix -= 1;
        } else {
            for (ix = -1022, i = (hx << 11); i > 0; i <<= 1) ix -= 1;
        }
    } else
        ix = (hx >> 20) - 1023;

    /* determine iy = ilogb(y) */
    if (hy < 0x00100000) { /* subnormal y */
        if (hy == 0) {
            for (iy = -1043, i = ly; i > 0; i <<= 1) iy -= 1;
        } else {
            for (iy = -1022, i = (hy << 11); i > 0; i <<= 1) iy -= 1;
        }
    } else
        iy = (hy >> 20) - 1023;

    /* set up {hx,lx}, {hy,ly} and align y to x */
    if (ix >= -1022)
        hx = 0x00100000 | (0x000fffff & hx);
    else { /* subnormal x, shift x to normal */
        n = -1022 - ix;
        if (n <= 31) {
            hx = (hx << n) | (lx >> (32 - n));
            lx <<= n;
        } else {
            hx = lx << (n - 32);
            lx = 0;
        }
    }
    if (iy >= -1022)
        hy = 0x00100000 | (0x000fffff & hy);
    else { /* subnormal y, shift y to normal */
        n = -1022 - iy;
        if (n <= 31) {
            hy = (hy << n) | (ly >> (32 - n));
            ly <<= n;
        } else {
            hy = ly << (n - 32);
            ly = 0;
        }
    }

    /* fix point fmod */
    n = ix - iy;
    while (n--) {
        hz = hx - hy;
        lz = lx - ly;
        if (lx < ly) hz -= 1;
        if (hz < 0) {
            hx = hx + hx + (lx >> 31);
            lx = lx + lx;
        } else {
            if ((hz | lz) == 0) /* return sign(x)*0 */
                return Zero[(u32) sx >> 31];
            hx = hz + hz + (lz >> 31);
            lx = lz + lz;
        }
    }
    hz = hx - hy;
    lz = lx - ly;
    if (lx < ly) hz -= 1;
    if (hz >= 0) {
        hx = hz;
        lx = lz;
    }

    /* convert back to floating value and restore the sign */
    if ((hx | lx) == 0) /* return sign(x)*0 */
        return Zero[(u32) sx >> 31];
    while (hx < 0x00100000) { /* normalize x */
        hx = hx + hx + (lx >> 31);
        lx = lx + lx;
        iy -= 1;
    }
    if (iy >= -1022) { /* normalize output */
        hx = ((hx - 0x00100000) | ((iy + 1023) << 20));
        INSERT_WORDS(x, hx | sx, lx);
    } else { /* subnormal output */
        n = -1022 - iy;
        if (n <= 20) {
            lx = (lx >> n) | ((u32) hx << (32 - n));
            hx >>= n;
        } else if (n <= 31) {
            lx = (hx << (32 - n)) | (lx >> n);
            hx = sx;
        } else {
            lx = hx >> (n - 32);
            hx = sx;
        }
        INSERT_WORDS(x, hx | sx, lx);
        x *= one; /* create necessary signal */
    }
    return x; /* exact output */
}

int toupper(int c) { return LSTD_NAMESPACE::to_upper(c); }
}
