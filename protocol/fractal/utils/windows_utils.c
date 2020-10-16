/**
 * Copyright Fractal Computers, Inc. 2020
 * @file windows_utils.c
 * @brief This file contains all code that interacts directly with Windows
 *        desktops (Winlogon, the login screen, and regular desktops).
============================
Usage
============================

DesktopContext: This type represents a Windows desktop object.
    - This object can be used to represent a Windows desktop to set to (for
      instance, Winsta0, the default Windows desktop)
    - It can also be used to tell whether the desktop is ready (for instance,
      Winlogon)
*/

#define _CRT_SECURE_NO_WARNINGS  // stupid Windows warnings

#include "windows_utils.h"

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
        LOG_WARNING("SetThreadDesktop failed w/ error code: %d.\n", GetLastError());
        return -2;
    }
    return 0;
}

DesktopContext OpenNewDesktop(WCHAR* desktop_name, bool get_name, bool set_thread) {
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
        GetUserObjectInformationW(new_desktop, UOI_NAME, szName, sizeof(szName), &dwLen);
        memcpy(context.desktop_name, szName, dwLen);
    }

    context.desktop_handle = new_desktop;
    CloseDesktop(new_desktop);

    return context;
}

void OpenWindow() {
    HWINSTA hwinsta = OpenWindowStationW(L"WinSta0", FALSE, GENERIC_ALL);
    SetProcessWindowStation(hwinsta);
}

// Log into the desktop, and block until the login process finishes
bool InitDesktop(input_device_t* input_device, char* vm_password) {
    DesktopContext lock_screen;

    OpenWindow();
    lock_screen = OpenNewDesktop(NULL, true, true);

    bool failed = false;
    int attempt = 0;
    while (wcscmp(L"Default", lock_screen.desktop_name) != 0) {
        LOG_INFO("Desktop name is %S\n", lock_screen.desktop_name);
        LOG_INFO("Attempting to log into desktop...\n");

        if (attempt > 10) {
            LOG_WARNING("Attempted too many times! Giving up...\n");
            failed = true;
            break;
        }

        // Setup typing area (Unknown true use?)

        FractalKeycode keycodes[] = {FK_SPACE, FK_BACKSPACE, FK_BACKSPACE};

        InputKeycodes(input_device, keycodes, 3);

        Sleep(500);

        // Type the vm password

        // Translate vm_password into keycodes
        int password_len = strlen(vm_password);
        FractalKeycode* password_keycodes = malloc(password_len);

        for (int i = 0; i < password_len; i++) {
            char c = vm_password[i];
            if ('a' <= c && c <= 'f') {
                password_keycodes[i] = FK_A + (c - 'a');
            } else if ('1' <= c && c <= '9') {
                password_keycodes[i] = FK_1 + (c - '1');
            } else if (c == '0') {
                password_keycodes[i] = FK_0;
            } else if (c == '.') {
                password_keycodes[i] = FK_PERIOD;
            } else {
                LOG_ERROR("CANNOT PARSE CHARACTER: %c (%d)", c, (int)c);
            }
        }

        // Type in the password
        EnterWinString(password_keycodes, password_len);

        free(password_keycodes);

        // Hit enter

        FractalKeycode keycodes[] = {FK_ENTER, FK_ENTER};

        EnterWinString(keycodes, 2);

        Sleep(1000);

        lock_screen = OpenNewDesktop(NULL, true, true);

        attempt++;
    }

    return !failed;
}
