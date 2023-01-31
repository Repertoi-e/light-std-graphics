export module g.video.monitor;

import "lstd.h";

export import g.video.monitor.general;

#if OS == WINDOWS
export import g.video.monitor.win32;
#else
#error Implement.
#endif
