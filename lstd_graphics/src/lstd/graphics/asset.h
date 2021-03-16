#pragma once

#include "lstd/memory/string.h"

LSTD_BEGIN_NAMESPACE

// We call an asset something with a name and a file path (file path is optional).
// Things like shader and texture inherit this.
struct asset {
    string Name, FilePath;
};

LSTD_END_NAMESPACE
