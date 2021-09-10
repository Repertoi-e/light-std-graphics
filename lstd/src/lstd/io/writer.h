#pragma once

#include "../common.h"

LSTD_BEGIN_NAMESPACE

// Provides a way to write types and bytes with a simple extension API.
// Subclasses of this stuct override the write/flush methods depending on the output (console, files, buffers, etc.)
// Types are written with the _write_ overloads.
struct writer {
    writer() {
    }

    virtual void write(const byte *data, s64 count) = 0;

    virtual void flush() {
    }
};

inline void write(writer *w, const byte *data, s64 size) { w->write(data, size); }
inline void write(writer *w, const bytes &data) { w->write(data.Data, data.Count); }
inline void write(writer *w, const string &str) { w->write((byte *) str.Data, str.Count); }

inline void write(writer *w, code_point cp) {
    utf8 data[4];
    utf8_encode_cp(data, cp);
    w->write((byte *) data, utf8_get_size_of_cp(data));
}

inline void flush(writer *w) { w->flush(); }

LSTD_END_NAMESPACE
