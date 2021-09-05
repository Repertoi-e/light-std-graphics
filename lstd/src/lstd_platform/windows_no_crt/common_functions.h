#pragma once

#include "lstd/common/common.h"

extern "C" {
const void *memchr(const void *s, int c, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
void *memcpy(void *dst, const void *src, size_t n);
void *memmove(void *dst, const void *src, size_t n);
void *memset(void *ptr, int value, size_t n);
char *strcat(char *s1, const char *s2);
int strcmp(const char *s1, const char *s2);
char *strcpy(char *dst, const char *src);
size_t strlen(const char *s);

const char *strchr(const char *s, int c);
const char *strstr(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
char *strncpy(char *dst, const char *src, size_t n);
const char *strrchr(const char *s, int n);

int strtol(const char *nptr, char **endptr, int base);
double atof(const char *str);
double strtod(const char *str, char **endptr);

double fmod(double x, double y);

int sscanf(const char *str, const char *fmt, ...);
int sprintf(char *str, const char *format, ...);

void qsort(void *data, size_t items, size_t size, int (*compare)(const void *, const void *));

int toupper(int c);
}

// Float versions of math functions (defined in cephes)
inline float fmodf(float x, float y) { return (float) fmod((double) x, (double) y); }
inline float powf(float x, float y) { return (float) pow((double) x, (double) y); }
inline float logf(float x) { return (float) log((double) x); }
inline float fabsf(float x) { return (float) LSTD_NAMESPACE::abs((double) x); }
inline float sqrtf(float x) { return (float) sqrt((double) x); }
inline float cosf(float x) { return (float) cos((double) x); }
inline float sinf(float x) { return (float) sin((double) x); }
inline float acosf(float x) { return (float) acos((double) x); }
inline float atan2f(float x, float y) { return (float) atan2((double) x, (double) y); }
inline float ceilf(float x) { return (float) ceil((double) x); }
