#pragma once

#include "../common.h"

LSTD_BEGIN_NAMESPACE

// Provides a way to write types and bytes with a simple extension API.
// Subclasses of this stuct override the write/flush methods depending on the output (console, files, buffers, etc.)
// Types are written with the _write_ overloads outside of this struct.
struct writer {
    virtual void write(const char *data, s64 count) = 0;
    virtual void flush() {}
};

inline void write(writer *w, string str) { w->write(str.Data, str.Count); }
inline void write(writer *w, const char *data, s64 size) { w->write(data, size); }

inline void write(writer *w, code_point cp) {
    char data[4];
    utf8_encode_cp(data, cp);
    w->write(data, utf8_get_size_of_cp(data));
}

//
// For more types (including formatting) see the lstd.fmt module and the fmt_context writer.
//

inline void flush(writer *w) { w->flush(); }

LSTD_END_NAMESPACE
