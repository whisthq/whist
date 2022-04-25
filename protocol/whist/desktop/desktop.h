#ifndef PROTOCOL_WHIST_DESKTOP_DESKTOP_H_
#define PROTOCOL_WHIST_DESKTOP_DESKTOP_H_

#include <whist/cursor/cursor.h>
#include <whist/utils/window_info.h>


typedef struct DesktopEnv DesktopEnv;
typedef void *(*DesktopGetClipboardHandlerCb)(DesktopEnv *desktop);
typedef void *(*DesktopGetWindowsHandlerCb)(DesktopEnv *desktop);
typedef void *(*DesktopUninitCb)(DesktopEnv *desktop);


/** @brief abstract desktop environment object */
struct  DesktopEnv {
	DesktopType kind;

	WhistCursorManager *cursors_handler;
	WhistWindowManager *windows_handler;

	//	DesktopGetClipboardHandlerCb get_clipboard_handler;
	DesktopUninitCb uninit;
};

/** Creates a desktop environment
 *
 * @param kind the kind of desktop environment to create
 * @param capture the capture device
 * @return the created DesktopEnv
 */
DesktopEnv* desktop_create(DesktopType kind, void* capture);

#endif /* PROTOCOL_WHIST_DESKTOP_DESKTOP_H_ */
