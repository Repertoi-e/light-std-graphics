#include "lstd/internal/context.h"
#include "lstd/memory/string.h"
#include "lstd_graphics/graphics.h"

#define STBI_ASSERT assert

#define STBI_MALLOC(size) (void *) LSTD_NAMESPACE::allocate_array<char>(size)
#define STBI_REALLOC(ptr, newSize) (ptr ? (void *) LSTD_NAMESPACE::reallocate_array(ptr, newSize) : (void *) LSTD_NAMESPACE::allocate_array<char>(newSize))

#define STBI_FREE(ptr) LSTD_NAMESPACE::free(ptr)

#define STBI_WINDOWS_UTF8
// #define STBI_NO_STDIO 
#define STBI_FAILURE_USERMSG

// The next 4 defines were added by us (we modified stb_image.h)
#define STBI_MEMSET LSTD_NAMESPACE::fill_memory
#define STBI_MEMCPY LSTD_NAMESPACE::copy_memory

#define STBI_STRCMP(x, y) LSTD_NAMESPACE::compare_c_string_lexicographically((const char *) x, (const char *) y)
#define STBI_STRNCMP(s1, s2, n) LSTD_NAMESPACE::compare_lexicographically(LSTD_NAMESPACE::string(s1, n), LSTD_NAMESPACE::string(s2, n))

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
