#include <whist/logging/logging.h>

#include "desktop.h"

DesktopEnv* x11_desktop_create(void *args);
DesktopEnv* weston_desktop_create(void *args);

DesktopEnv* desktop_create(DesktopType kind, void* capture)
{
	switch (kind) {
	case DESKTOP_X11:
		return x11_desktop_create(capture);
	case DESKTOP_WESTON:
		return weston_desktop_create(capture);
	default:
		FATAL_ASSERT("unhandled type");
	}

	return NULL;
}



