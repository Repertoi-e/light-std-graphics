// We build freetype without the CRT
// so we need to provide replacement functions.

#include "lstd/common/context.h"
#include "lstd_platform/windows_no_crt/common_functions.h"

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

void *ft_scalloc(size_t num, size_t size) {
    void *r = (void *) LSTD_NAMESPACE::allocate_array<byte>(num * size);
    ft_memset(r, 0, num * size);
    return r;
}

void ft_sfree(void *block) { LSTD_NAMESPACE::free(block); }
void *ft_smalloc(size_t size) { return (void *) LSTD_NAMESPACE::allocate_array<byte>(size); }
void *ft_srealloc(void *b, size_t size) { return (void *) LSTD_NAMESPACE::reallocate_array<byte>((byte *) b, size); }

const char *ft_strchr(const char *str, int character) { return strchr(str, character); }
const char *ft_strstr(const char *str1, const char *str2) { return strstr(str1, str2); }
int ft_strncmp(const char *str1, const char *str2, size_t num) { return strncmp(str1, str2, num); }
char *ft_strncpy(char *destination, const char *source, size_t num) { return strncpy(destination, source, num); }
const char *ft_strrchr(const char *c, int d) { return strrchr(c, d); }
}
