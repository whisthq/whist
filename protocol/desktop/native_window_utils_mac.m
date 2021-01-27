#include "native_window_utils.h"
#include <Cocoa/Cocoa.h>
#include "SDL_syswm.h"

int set_native_window_color(SDL_Window *window, uint red, uint green, uint blue) {
    SDL_SysWMinfo sys_info = {0};
    SDL_GetWindowWMInfo(window, &sys_info);
    NSWindow *native_window = sys_info.info.cocoa.window;

    float red_percent = (float) red / 255.0;
    float green_percent = (float) green / 255.0;
    float blue_percent = (float) blue / 255.0;
    [native_window setTitlebarAppearsTransparent: transparency];
    [native_window setAppearance: [NSAppearance appearanceNamed:NSAppearanceNameVibrantLight]];
    [native_window setBackgroundColor: [NSColor colorWithDeviceRed: red_percent green: green_percent blue: blue_percent alpha: 1.0]];
    return 0;
}
