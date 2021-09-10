module;

#include "lstd/common/windows.h"  // Declarations of Win32 functions

//
// Simple wrapper around dynamic libraries and getting addresses of procedures.
//

export module lstd.os.win32.dynamic_library;

import lstd.os.win32.memory;

LSTD_BEGIN_NAMESPACE

export {
    struct dynamic_library_t {
    };

    using dynamic_library = dynamic_library_t *;

    dynamic_library os_dynamic_library_load(const string &path) {
        return (dynamic_library) LoadLibraryW(internal::platform_utf8_to_utf16(path, internal::platform_get_temporary_allocator()));
    }

    // :OverloadFree: We follow the convention to overload the "free" function
    // as a standard way to release resources (may not be just memory blocks).
    void free(dynamic_library library) {
        if (library) {
            FreeLibrary((HMODULE) library);
        }
    }

    void *os_dynamic_library_get_symbol(dynamic_library library, const char *name) {
        return (void *) GetProcAddress((HMODULE) library, name);
    }
}

LSTD_END_NAMESPACE
