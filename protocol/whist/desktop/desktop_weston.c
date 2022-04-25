
#include "desktop_weston.h"


#include <whist/core/whist_memory.h>

typedef struct {
	DesktopEnv base;

} WestonDesktopEnv;


DesktopEnv* weston_desktop_create(void *args) {
	DesktopEnv *ret = safe_zalloc(sizeof(*ret));

	ret->kind = DESKTOP_WESTON;
	ret->cursors_handler = whist_cursor_capture_init(WHIST_CURSOR_WESTON, args);
	return ret;
}
