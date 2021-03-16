#include "lstd/internal/context.h"
#include "lstd/memory/stack_array.h"

// #define STBRP_SORT @DependencyCleanup
#define STBRP_ASSERT assert

#define STBRP_SORT LSTD_NAMESPACE::quick_sort

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"
