#include "common_functions.h"

#include "lstd/memory/string.h"

//
// Some implementations of these are taken from:
// https://github.com/beloff-ZA/LIBFT/blob/master/libft.h
//
/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   libft.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: beloff <marvin@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2018/06/13 02:38:01 by beloff            #+#    #+#             */
/*   Updated: 2018/06/13 05:31:09 by beloff           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

extern "C" {

// Declare these as functions, not intrinsics. We have our optimized versions.
// See comment below.
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
u64 strlen(const char *s) {
    int i;

    i = 0;
    while (s[i] != '\0')
        i++;
    return (i);
}

int strcmp(const char *s1, const char *s2) {
    int i;

    i = 0;
    while (s1[i] != '\0' && s2[i] != '\0' && s1[i] == s2[i])
        i++;
    return ((unsigned char) s1[i] - (unsigned char) s2[i]);
}

int memcmp(const void *s1, const void *s2, size_t n) {
    unsigned char *p1;
    unsigned char *p2;
    size_t i;

    p1 = (unsigned char *) s1;
    p2 = (unsigned char *) s2;
    i  = 0;
    while (i < n) {
        if (p1[i] != p2[i])
            return (p1[i] - p2[i]);
        ++i;
    }
    return (0);
}

char *strcpy(char *dst, const char *src) {
    int i;

    i = 0;
    while (src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
    return (dst);
}

const void *memchr(const void *s, int c, size_t n) {
    size_t i;

    i = 0;
    while (n--) {
        if (((unsigned char *) s)[i] == (unsigned char) c)
            return ((void *) &((unsigned char *) s)[i]);
        i++;
    }
    return (NULL);
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
        uxi &= (u64) (-1) >> 12;
        uxi |= 1ULL << 52;
    }
    if (!ey) {
        for (i = uy.i << 12; i >> 63 == 0; ey--, i <<= 1)
            ;
        uy.i <<= -ey + 1;
    } else {
        uy.i &= (u64) (-1) >> 12;
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
#endif  // End comment about functions being optimized in Release

char *strcat(char *s1, const char *s2) {
    int i;
    int j;

    i = 0;
    j = 0;
    while (s1[i] != '\0') {
        i++;
    }
    while (s2[j] != '\0') {
        s1[i + j] = s2[j];
        j++;
    }
    s1[i + j] = '\0';
    return (s1);
}

const char *strstr(const char *haystack, const char *needle) {
    int i;
    int j;

    if (needle[0] == '\0')
        return ((char *) haystack);
    i = 0;
    while (haystack[i] != '\0') {
        j = 0;
        while (needle[j] != '\0') {
            if (haystack[i + j] != needle[j])
                break;
            j++;
        }
        if (needle[j] == '\0')
            return ((char *) haystack + i);
        i++;
    }
    return (0);
}

const char *strchr(const char *s, int c) {
    while (*s != (char) c && *s != '\0')
        s++;
    if (*s == (char) c)
        return ((char *) s);
    return (NULL);
}

int strncmp(const char *s1, const char *s2, size_t n) {
    size_t i;

    if (!n)
        return (0);
    i = 1;
    while (*s1 == *s2) {
        if (!*s1++ || i++ == n)
            return (0);
        s2++;
    }
    return ((unsigned char) (*s1) - (unsigned char) (*s2));
}

char *strncpy(char *dst, const char *src, size_t len) {
    size_t i;

    i = 0;
    while (src[i] && i < len) {
        dst[i] = src[i];
        i++;
    }
    while (i < len)
        dst[i++] = '\0';
    return (dst);
}

int toupper(int c) { return LSTD_NAMESPACE::to_upper(c); }

static unsigned char charmap(char c) {
    char chr;

    chr = toupper(c);
    if (chr >= '0' && chr <= '9')
        return (chr - '0');
    if (chr >= 'A' && chr <= 'Z')
        return (chr - 'A' + 10);
    return (127);
}

static int getbase(const char **nptr, int base) {
    const char *ptr;

    ptr = *nptr;
    if ((base == 0 || base == 16) && *ptr == '0') {
        if (*(++ptr) == 'x' && (++(ptr)))
            base = 16;
        else if (*ptr == '0')
            base = 8;
        else
            base = 10;
        *nptr = ptr;
    }
    return (base);
}

int strtol(const char *nptr, char **endptr, int base) {
    int neg;
    long result;
    char digit;

    if (base < 0 || base > 36)
        return (0);
    neg    = 0;
    result = 0;
    while (LSTD_NAMESPACE::is_space(*nptr))
        nptr++;
    if (*nptr == '-' || *nptr == '+')
        if (*nptr++ == '-')
            neg = 1;
    base = getbase(&nptr, base);
    while ((digit = charmap(*nptr++)) < base)
        if ((result = (result * base) + digit) < 0) {
            if (endptr)
                *endptr = (char *) nptr;
            return (S32_MAX + neg);
        }
    if (endptr)
        *endptr = (char *) nptr;
    return (result + (result * -2 * neg));
}

double atof(const char *str) {
    return 0.0;
}

int sscanf(const char *str, const char *fmt, ...) {
    return 0;
}
}
