#include "native_window_utils.h"
#include <Cocoa/Cocoa.h>
#include "SDL_syswm.h"

int set_native_window_color(SDL_Window *window, uint red, uint green, uint blue) {
    /*



    */
    SDL_SysWMinfo sys_info = {0};
    SDL_GetWindowWMInfo(window, &sys_info);
    NSWindow *native_window = sys_info.info.cocoa.window;

    float red_percent = (float) red / 255.0;
    float green_percent = (float) green / 255.0;
    float blue_percent = (float) blue / 255.0;

    // based on https://stackoverflow.com/a/3943023
    if (red_percent * 0.2126 + green_percent * 0.7152 + blue_percent * 0.0722 > 0.179) {
        // dark font color for titlebar
        [native_window setAppearance: [NSAppearance appearanceNamed:NSAppearanceNameVibrantLight]];
    } else {
        // light font color for titlebar
        [native_window setAppearance: [NSAppearance appearanceNamed:NSAppearanceNameVibrantDark]];
    }

    const CGFloat components[4] = {red_percent, green_percent, blue_percent, 1.0};

    [native_window setTitlebarAppearsTransparent: true];
    [native_window setBackgroundColor: [NSColor colorWithColorSpace: [[native_window screen] colorSpace]  components: &components[0] count: 4]];
    return 0;
}
