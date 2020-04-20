#include "linuxcursor.h"  // header file for this file

static Display* disp;

static uint32_t last_cursor[MAX_CURSOR_WIDTH * MAX_CURSOR_HEIGHT] = {0};

void InitCursors() { disp = XOpenDisplay(NULL); }

FractalCursorImage GetCurrentCursor() {
    FractalCursorImage image = {0};
    image.cursor_id = SDL_SYSTEM_CURSOR_ARROW;
    image.cursor_state = CURSOR_STATE_VISIBLE;
    if (disp) {
        XLockDisplay(disp);
        XFixesCursorImage* ci = XFixesGetCursorImage(disp);
        XUnlockDisplay(disp);
        image.cursor_bmp_width = ci->width;
        image.cursor_bmp_height = ci->height;
        image.cursor_bmp_hot_x = ci->xhot;
        image.cursor_bmp_hot_y = ci->yhot;

        for (int k = 0; k < ci->width * ci->height; ++k) {
            // we need to do this in case ci->pixels uses 8 bytes per pixel
            uint32_t argb = (uint32_t)ci->pixels[k];
            // unsigned int a, r, g, b;
            // a = (argb & 0xff000000) >> 24;
            // if (!a) continue;
            // r = (argb & 0x00ff0000) >> 16;
            // g = (argb & 0x0000ff00) >> 8;
            // b = (argb & 0x000000ff);
            // r = min((255 * r) / a, 255);
            // g = min((255 * g) / a, 255);
            // b = min((255 * b) / a, 255);
            // argb = (a << 24) + (r << 16) + (g << 8) + b;
            image.cursor_bmp[k] = argb;
        }

        if (memcmp(image.cursor_bmp, last_cursor,
                   sizeof(uint32_t) * MAX_CURSOR_WIDTH * MAX_CURSOR_HEIGHT)) {
            image.cursor_use_bmp = true;
            memcpy(last_cursor, image.cursor_bmp,
                   sizeof(uint32_t) * MAX_CURSOR_WIDTH * MAX_CURSOR_HEIGHT);
        } else {
            image.cursor_use_bmp = false;
        }

        XFree(ci);
    }

    return image;
}
// typedef struct FractalCursorTypes {
//     HCURSOR CursorAppStarting;
//     HCURSOR CursorArrow;
//     HCURSOR CursorCross;
//     HCURSOR CursorHand;
//     HCURSOR CursorHelp;
//     HCURSOR CursorIBeam;
//     HCURSOR CursorIcon;
//     HCURSOR CursorNo;
//     HCURSOR CursorSize;
//     HCURSOR CursorSizeAll;
//     HCURSOR CursorSizeNESW;
//     HCURSOR CursorSizeNS;
//     HCURSOR CursorSizeNWSE;
//     HCURSOR CursorSizeWE;
//     HCURSOR CursorUpArrow;
//     HCURSOR CursorWait;
// } FractalCursorTypes;

// struct FractalCursorTypes l_types = {0};
// struct FractalCursorTypes* types = &l_types;

// FractalCursorImage GetCursorImage(PCURSORINFO pci);

// void initCursors() { LoadCursors(); }

// void LoadCursors() {
//     types->CursorAppStarting = LoadCursor(NULL, IDC_APPSTARTING);
//     types->CursorArrow = LoadCursor(NULL, IDC_ARROW);
//     types->CursorCross = LoadCursor(NULL, IDC_CROSS);
//     types->CursorHand = LoadCursor(NULL, IDC_HAND);
//     types->CursorHelp = LoadCursor(NULL, IDC_HELP);
//     types->CursorIBeam = LoadCursor(NULL, IDC_IBEAM);
//     types->CursorIcon = LoadCursor(NULL, IDC_ICON);
//     types->CursorNo = LoadCursor(NULL, IDC_NO);
//     types->CursorSize = LoadCursor(NULL, IDC_SIZE);
//     types->CursorSizeAll = LoadCursor(NULL, IDC_SIZEALL);
//     types->CursorSizeNESW = LoadCursor(NULL, IDC_SIZENESW);
//     types->CursorSizeNS = LoadCursor(NULL, IDC_SIZENS);
//     types->CursorSizeNWSE = LoadCursor(NULL, IDC_SIZENWSE);
//     types->CursorSizeWE = LoadCursor(NULL, IDC_SIZEWE);
//     types->CursorUpArrow = LoadCursor(NULL, IDC_UPARROW);
//     types->CursorWait = LoadCursor(NULL, IDC_WAIT);
// }

// FractalCursorImage GetCursorImage(PCURSORINFO pci) {
//     HCURSOR cursor = pci->hCursor;
//     FractalCursorImage image = {0};

//     if (cursor == types->CursorArrow) {
//         image.cursor_id = SDL_SYSTEM_CURSOR_ARROW;
//     } else if (cursor == types->CursorCross) {
//         image.cursor_id = SDL_SYSTEM_CURSOR_CROSSHAIR;
//     } else if (cursor == types->CursorHand) {
//         image.cursor_id = SDL_SYSTEM_CURSOR_HAND;
//     } else if (cursor == types->CursorIBeam) {
//         image.cursor_id = SDL_SYSTEM_CURSOR_IBEAM;
//     } else if (cursor == types->CursorNo) {
//         image.cursor_id = SDL_SYSTEM_CURSOR_NO;
//     } else if (cursor == types->CursorSizeAll) {
//         image.cursor_id = SDL_SYSTEM_CURSOR_SIZEALL;
//     } else if (cursor == types->CursorSizeNESW) {
//         image.cursor_id = SDL_SYSTEM_CURSOR_SIZENESW;
//     } else if (cursor == types->CursorSizeNS) {
//         image.cursor_id = SDL_SYSTEM_CURSOR_SIZENS;
//     } else if (cursor == types->CursorSizeNWSE) {
//         image.cursor_id = SDL_SYSTEM_CURSOR_SIZENWSE;
//     } else if (cursor == types->CursorSizeWE) {
//         image.cursor_id = SDL_SYSTEM_CURSOR_SIZEWE;
//     } else if (cursor == types->CursorWait) {
//         image.cursor_id = SDL_SYSTEM_CURSOR_WAITARROW;
//     } else {
//         image.cursor_id = SDL_SYSTEM_CURSOR_ARROW;
//     }

//     return image;
// }

// FractalCursorImage GetCurrentCursor() {
//     CURSORINFO pci;
//     pci.cbSize = sizeof(CURSORINFO);
//     GetCursorInfo(&pci);

//     FractalCursorImage image = {0};
//     image = GetCursorImage(&pci);

//     image.cursor_state = pci.flags;

//     return image;
// }