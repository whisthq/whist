#include "window_info.h"

#include "x11_window_info.h"

WhistWindowManager* window_manager_get(DesktopType kind, void *args)
{
	switch(kind)
	{
	case DESKTOP_X11:
		return x11_window_manager(args);
	case DESKTOP_WESTON:
	case DESKTOP_WIN32:
	default:
		LOG_FATAL("unimplemented window lister type");
		return NULL;
	}
}
