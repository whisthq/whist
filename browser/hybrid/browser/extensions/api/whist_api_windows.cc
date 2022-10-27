
extern "C" {
#include <windows.h>
#include <winreg.h>
}
#include <algorithm>
#include "whist_api_native.h"

#include <iostream>

// These are the default values to be used on Windows.
#define DEFAULT_KEY_REPEAT_INIT_DELAY 1
#define DEFAULT_KEY_REPEAT_RATE 31

int native_get_keyboard_repeat_rate() {
    char buf[256];
    DWORD sz = sizeof(buf);
    LSTATUS ret = RegGetValueW(
        HKEY_CURRENT_USER,
        L"Control Panel\\Keyboard\\",
        L"KeyboardSpeed",
        RRF_RT_REG_SZ,
        nullptr,
        (void*)buf,
        &sz
    );
    int repeat_rate = DEFAULT_KEY_REPEAT_RATE;
    if (ret == ERROR_SUCCESS) {
        wchar_t* end;
        // Convert wstring to integer
        repeat_rate = (int)std::wcstol((const wchar_t*)buf, &end, 10);
    }
    // https://learn.microsoft.com/en-us/dotnet/api/system.windows.forms.systeminformation.keyboardspeed
    return std::clamp(repeat_rate - 1, 2, 60);
}
    
int native_get_keyboard_repeat_initial_delay() {
    char buf[256];
    DWORD sz = sizeof(buf);
    LSTATUS ret = RegGetValueW(
        HKEY_CURRENT_USER,
        L"Control Panel\\Keyboard\\",
        L"KeyboardDelay",
        RRF_RT_REG_SZ,
        nullptr,
        (void*)buf,
        &sz
    );
    int repeat_initial_delay = DEFAULT_KEY_REPEAT_INIT_DELAY;
    if (ret == ERROR_SUCCESS) {
        wchar_t* end;
        // Convert wstring to integer
        repeat_initial_delay = (int)std::wcstol((const wchar_t*)buf, &end, 10);
    }
    // https://learn.microsoft.com/en-us/dotnet/api/system.windows.forms.systeminformation.keyboarddelay
    return 250 + 250 * std::clamp(repeat_initial_delay, 0, 6);
}

void native_get_system_language(char* locale_str_buffer, int buffer_size) {
    locale_str_buffer[0] = '\0';
}
