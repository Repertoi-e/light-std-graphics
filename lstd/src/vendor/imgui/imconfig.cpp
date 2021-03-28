#include "imconfig.h"

extern "C" {

#pragma function(memset)
void *memset(void *ptr, int value, size_t n) { return LSTD_NAMESPACE::fill_memory(ptr, value, n); }
#pragma function(memcpy)
void *memcpy(void *dest, const void *src, size_t n) { return LSTD_NAMESPACE::copy_memory(dest, src, n); }
#pragma function(memmove)
void *memmove(void *dest, const void *src, size_t n) { return LSTD_NAMESPACE::copy_memory(dest, src, n); }

// If we are building with MSVC in Release, the compiler optimizes the following functions as instrinsics.
// In that case we don't define them, because these are slow anyway. Similar thing happens with some math
// functions in the Cephes library (search for :WEMODIFIEDCEPHES:)
#if COMPILER == MSVC and not defined NDEBUG
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

double fmod(double x, double y) {
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
#endif

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

int toupper(int c) { return LSTD_NAMESPACE::to_upper(c); }
}
