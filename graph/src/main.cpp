#include <lstd/video.h>

import fmt;

using namespace lstd;

s32 main() {
    auto monitors = os_get_monitors();
    For(monitors) {
        print("MONITOR: {}\n", it->Name);
        For_as(d, it->DisplayModes) {
            print("   DISPLAY MODE: {}x{}, refresh rate: {}, RGB: {} {} {}\n", d.Width, d.Height, d.RefreshRate, d.RedBits, d.GreenBits, d.BlueBits);
        }
    }
}
