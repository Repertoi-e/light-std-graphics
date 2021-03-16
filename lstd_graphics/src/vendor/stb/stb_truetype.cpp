#include "lstd/internal/context.h"
#include "lstd/memory/string_utils.h"

#define STBTT_malloc(x, u) ((void) (u), LSTD_NAMESPACE::allocate_array<char>(x))
#define STBTT_free(x, u) ((void) (u), LSTD_NAMESPACE::free(x))
#define STBTT_assert(x) assert(x)
#define STBTT_strlen(x) LSTD_NAMESPACE::c_string_length(x)

#define STBTT_memcpy LSTD_NAMESPACE ::copy_memory
#define STBTT_memset LSTD_NAMESPACE ::fill_memory

#define STBTT_ifloor(x) ((int) ImFloorStd(x))
#define STBTT_iceil(x) ((int) ImCeil(x))

#define STBTT_sqrt(x) ImSqrt(x)
#define STBTT_pow(x, y) ImPow(x, y)

#define STBTT_fmod(x, y) ImFmod(x, y)

#define STBTT_cos(x) ImCos(x)
#define STBTT_acos(x) ImAcos(x)

#define STBTT_fabs(x) ImFabs(x)

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
