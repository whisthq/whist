#define _CRT_SECURE_NO_WARNINGS  // stupid Windows warnings
#include "desktop.h"

void logToFile(char* msg, char* filename) {
    FILE* fp;
    fp = fopen(filename, "a+");
    fprintf(fp, msg);
    printf(msg);
    fclose(fp);
}

// @brief Attaches the current thread to the current input desktop.
// @details Uses OpenInputDesktop and SetThreadDesktop from WinAPI.
int setCurrentInputDesktop(HDESK currentInputDesktop) {
    // Set current thread to the current user input desktop
    if (!SetThreadDesktop(currentInputDesktop)) {
        mprintf("SetThreadDesktop failed w/ error code: %d.\n", GetLastError());
        return -2;
    }
    return 0;
}

// Log into the desktop, and block until the login process finishes
DesktopContext OpenNewDesktop(char* desktop_name, bool get_name,
                              bool set_thread) {
    DesktopContext context = {0};
    HDESK new_desktop;

    if (desktop_name == NULL) {
        new_desktop = OpenInputDesktop(0, FALSE, GENERIC_ALL);
    } else {
        new_desktop = OpenDesktop((LPCWSTR)desktop_name, 0, FALSE, GENERIC_ALL);
    }

    if (set_thread) {
        setCurrentInputDesktop(new_desktop);
    }

    if (get_name) {
        TCHAR szName[1000];
        DWORD dwLen;
        GetUserObjectInformation(new_desktop, UOI_NAME, szName, sizeof(szName),
                                 &dwLen);
        memcpy(context.desktop_name, szName, dwLen);
    }

    context.desktop_handle = new_desktop;
    CloseDesktop(new_desktop);

    return context;
}

void OpenWindow() {
    HWINSTA hwinsta =
        OpenWindowStation((LPCWSTR) "winsta0", FALSE, GENERIC_ALL);
    SetProcessWindowStation(hwinsta);
}

void InitDesktop() {
    DesktopContext lock_screen;

    OpenWindow();
    lock_screen = OpenNewDesktop(NULL, true, true);

    while (strcmp((const char*)L"Default",
                  (const char*)lock_screen.desktop_name) != 0) {
        mprintf("Desktop name is %s\n", lock_screen.desktop_name);
        mprintf("Attempting to log into desktop...\n");

        enum FractalKeycode keycodes[] = {FK_SPACE, FK_BACKSPACE, FK_BACKSPACE};

        EnterWinString(keycodes, 3);

        Sleep(500);

        enum FractalKeycode keycodes2[] = {
            FK_P, FK_A, FK_S, FK_S, FK_W, FK_O, FK_R,      FK_D,     FK_1,
            FK_2, FK_3, FK_4, FK_5, FK_6, FK_7, FK_PERIOD, FK_ENTER, FK_ENTER};

        EnterWinString(keycodes2, 18);

        Sleep(500);

        lock_screen = OpenNewDesktop(NULL, true, true);
    }
}
