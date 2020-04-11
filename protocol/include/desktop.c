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

DesktopContext OpenNewDesktop(WCHAR* desktop_name, bool get_name,
                              bool set_thread) {
    DesktopContext context = {0};
    HDESK new_desktop;

    if (desktop_name == NULL) {
        new_desktop = OpenInputDesktop(0, FALSE, GENERIC_ALL);
    } else {
        new_desktop = OpenDesktopW(desktop_name, 0, FALSE, GENERIC_ALL);
    }

    if (set_thread) {
        setCurrentInputDesktop(new_desktop);
    }

    if (get_name) {
        TCHAR szName[1000];
        DWORD dwLen;
        GetUserObjectInformationW(new_desktop, UOI_NAME, szName, sizeof(szName),
                                 &dwLen);
        memcpy(context.desktop_name, szName, dwLen);
    }

    context.desktop_handle = new_desktop;
    CloseDesktop(new_desktop);

    return context;
}

void OpenWindow() {
    HWINSTA hwinsta =
        OpenWindowStationW(L"WinSta0", FALSE, GENERIC_ALL);
    SetProcessWindowStation(hwinsta);
}

// Log into the desktop, and block until the login process finishes
void InitDesktop() {
    DesktopContext lock_screen;

    OpenWindow();
    lock_screen = OpenNewDesktop(NULL, true, true);

    int attempt = 0;
    while (wcscmp(L"Default",
                  lock_screen.desktop_name) != 0) {
        mprintf("Desktop name is %S\n", lock_screen.desktop_name);
        mprintf("Attempting to log into desktop...\n");

        if( attempt > 10 )
        {
            mprintf( "Attempted too many times! Giving up...\n" );
            break;
        }

        enum FractalKeycode keycodes[] = {FK_SPACE, FK_BACKSPACE, FK_BACKSPACE};

        EnterWinString(keycodes, 3);

        Sleep(500);

        enum FractalKeycode keycodes2[] = {
            FK_P, FK_A, FK_S, FK_S, FK_W, FK_O, FK_R,      FK_D,     FK_1,
            FK_2, FK_3, FK_4, FK_5, FK_6, FK_7, FK_PERIOD, FK_ENTER, FK_ENTER};

        EnterWinString(keycodes2, 18);

        Sleep(500);

        lock_screen = OpenNewDesktop(NULL, true, true);

        attempt++;
    }
}
