
#include <whist/core/whist_memory.h>
#include "desktop_x11.h"



DesktopEnv* x11_desktop_create(void *args) {
	DesktopEnv *ret = safe_zalloc(sizeof(*ret));

	ret->kind = DESKTOP_X11;
	ret->cursors_handler = whist_cursor_capture_init(WHIST_CURSOR_X11, args);
	ret->windows_handler = window_manager_get(DESKTOP_X11, args);
	return ret;
}
