//
// We build freetype without the CRT
// so we need to provide replacement functions.
//

#include "lstd/platform/windows_no_crt/common_functions.h"
#include "lstd-graphics/third_party/stb/stb_sprintf.h"

import lstd.os;
import lstd.string;

extern "C" {
const void *ft_memchr(const void *c, int d, size_t e) { return memchr(c, d, e); }
int ft_memcmp(const void *ptr1, const void *ptr2, size_t num) { return memcmp(ptr1, ptr2, num); }
void *ft_memcpy(void *dest, const void *src, size_t num) { return memcpy(dest, src, num); }
void *ft_memmove(void *dest, const void *src, size_t num) { return memmove(dest, src, num); }
void *ft_memset(void *ptr, int value, size_t num) { return memset(ptr, value, num); }
char *ft_strcat(char *destination, const char *source) { return strcat(destination, source); }
int ft_strcmp(const char *str1, const char *str2) { return strcmp(str1, str2); }
char *ft_strcpy(char *destination, const char *source) { return strcpy(destination, source); }
u64 ft_strlen(const char *str) { return strlen(str); }

int ft_strtol(const char *nptr, char **endptr, int base) { return strtol(nptr, endptr, base); }

void *ft_scalloc(size_t num, size_t size) { return calloc(num, size); }
void *ft_smalloc(size_t size) { return malloc(size); }
void *ft_srealloc(void *b, size_t size) { return realloc(b, size); }
void ft_sfree(void *block) { free(block); }

char *ft_getenv(const char *var) {
    auto [result, success] = LSTD_NAMESPACE::os_get_env(var, true);  // @Leak
    if (!success) return null;

    // Double @Leak
    return LSTD_NAMESPACE::to_c_string(result, LSTD_NAMESPACE::platform_get_persistent_allocator());
}

int ft_sprintf(char *str, const char *format, ...) {
    int result;
    va_list va;
    va_start(va, format);
    result = stbsp_vsprintfcb(0, 0, str, format, va);
    va_end(va);
    return result;
}

const char *ft_strchr(const char *str, int character) { return strchr(str, character); }
const char *ft_strstr(const char *str1, const char *str2) { return strstr(str1, str2); }
int ft_strncmp(const char *str1, const char *str2, size_t num) { return strncmp(str1, str2, num); }
char *ft_strncpy(char *destination, const char *source, size_t num) { return strncpy(destination, source, num); }
const char *ft_strrchr(const char *c, int d) { return strrchr(c, d); }

void ft_qsort(void *data, size_t items, size_t size, int (*compare)(const void *, const void *)) { return qsort(data, items, size, compare); }
}
