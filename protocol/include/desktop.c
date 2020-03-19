#include "desktop.h"

void logToFile(char* msg, char* filename) {
    FILE* fp;
    fopen_s(&fp, filename, "a+");
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
DesktopContext OpenNewDesktop(char* desktop_name, bool get_name, bool set_thread) {
    DesktopContext context = { 0 };
    HDESK new_desktop;

    if (desktop_name == NULL) {
        new_desktop = OpenInputDesktop(0, FALSE, GENERIC_ALL);
    }
    else {
        new_desktop = OpenDesktop((LPCWSTR) desktop_name, 0, FALSE, GENERIC_ALL);
    }

    if (set_thread) {
        setCurrentInputDesktop(new_desktop);
    }

    if (get_name) {
        TCHAR szName[1000];
        DWORD dwLen;
        GetUserObjectInformation(new_desktop, UOI_NAME, szName, sizeof(szName), &dwLen);
        memcpy(context.desktop_name, szName, dwLen);
    }

    context.desktop_handle = new_desktop;
    CloseDesktop(new_desktop);

    return context;
}

void OpenWindow() {
    HWINSTA hwinsta = OpenWindowStation((LPCWSTR) "winsta0", FALSE, GENERIC_ALL);
    SetProcessWindowStation(hwinsta);
}

void InitDesktop() {
    DesktopContext lock_screen;

    OpenWindow();
    lock_screen = OpenNewDesktop(NULL, true, true);

    while (strcmp((const char *) L"Default", (const char *) lock_screen.desktop_name) != 0)
    {
        mprintf("Desktop name is %s\n", lock_screen.desktop_name);
        mprintf("Attempting to log into desktop...\n");

        enum FractalKeycode keycodes[] = {
          KEY_SPACE, KEY_BACKSPACE, KEY_BACKSPACE
        };

        EnterWinString(keycodes, 3);

        Sleep(500);

        enum FractalKeycode keycodes2[] = {
          KEY_P, KEY_A, KEY_S, KEY_S, KEY_W, KEY_O, KEY_R, KEY_D, KEY_1,
          KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_PERIOD, KEY_ENTER,
          KEY_ENTER
        };

        EnterWinString(keycodes2, 18);

        Sleep(500);

        lock_screen = OpenNewDesktop(NULL, true, true);
    }
}
