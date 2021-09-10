#pragma once

#include "lstd/common.h"

// @Cleanup: Decouple lstd lstd-graphics

LSTD_BEGIN_NAMESPACE
void win64_monitor_uninit();
void win64_window_uninit();
LSTD_END_NAMESPACE

// We can't forward-declare these in the module because they get mangled names.
