export module g.video.window;

import "lstd.h";

export import g.video.window.general;

#if OS == WINDOWS
export import g.video.window.win32;
#else
#error Implement.
#endif
