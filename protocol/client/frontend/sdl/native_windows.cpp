extern "C" {
#include "native.h"
#include <SDL2/SDL_syswm.h>
}
#include "sdl_struct.hpp"

#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#include <string>
#include <format>
#include <locale>
#include <codecvt>

#include <shlobj.h>
#include <pathcch.h>
#include <knownfolders.h>
#include <propvarutil.h>
#include <propkey.h>
#include <NotificationActivationCallback.h>
#include <Windows.UI.Notifications.h>

#include "windows_notification_helper.h"

static HWND get_native_window(SDL_Window* window) {
    SDL_SysWMinfo sys_info = {};
    SDL_GetWindowWMInfo(window, &sys_info);
    return sys_info.info.win.window;
}

// Already handled by SDL
void sdl_native_hide_taskbar(void) {}

void sdl_native_init_window_options(SDL_Window* window) {}

int sdl_native_set_titlebar_color(SDL_Window* window, const WhistRGBColor* color) {
    LOG_INFO("Not implemented on Windows.");
    return 0;
}

int sdl_native_get_dpi(SDL_Window* window) {
    HWND native_window = get_native_window(window);
    return (int)GetDpiForWindow(native_window);
}

void sdl_native_declare_user_activity(void) {
    // TODO: Implement actual wake-up code
    // For now, we'll just disable the screensaver
    SDL_DisableScreenSaver();
}

void sdl_native_init_external_drag_handler(WhistFrontend* frontend) {
    UNUSED(frontend);
    LOG_INFO("Not implemented on Windows");
}

void sdl_native_destroy_external_drag_handler(WhistFrontend* frontend) {
    UNUSED(frontend);
    LOG_INFO("Not implemented on Windows");
}

using namespace Microsoft::WRL;

// Required registry keys to enable notification callback delivery:
// [HKEY_LOCAL_MACHINE\SOFTWARE\Classes\AppUserModelId\Whist.Client.1]
// "DisplayName"="Whist Client"
// "CustomActivator"="{2765a8cd-b8d2-430d-b97b-4e778874d032}"
static const wchar_t* const whist_client_aumid = L"Whist.Client.1";

// An alternative mechanism to use here is to have a start menu shortcut
// with properties enabling notification delivery.  This is needed if we
// want notifications to be able to relaunch the client, but otherwise
// the registry method above is more convenient so this approach is
// currently disabled.
static bool start_menu_entry = false;

static SDLFrontendContext* event_context = NULL;

class DECLSPEC_UUID("2765a8cd-b8d2-430d-b97b-4e778874d032") NotificationActivator WrlSealed WrlFinal
    : public RuntimeClass<RuntimeClassFlags<ClassicCom>, INotificationActivationCallback> {
   public:
    virtual HRESULT STDMETHODCALLTYPE Activate(LPCWSTR aumid, LPCWSTR args,
                                               const NOTIFICATION_USER_INPUT_DATA* data,
                                               ULONG data_count) override {
        LOG_INFO("Notification activated: AUMID \"%ls\" args \"%ls\".", aumid, args);
        if (event_context) {
            SDL_Event event = {
                .user =
                    {
                        .type = event_context->internal_event_id,
                        .code = SDL_FRONTEND_EVENT_NOTIFICATION_CALLBACK,
                    },
            };
            SDL_PushEvent(&event);
        }
        return S_OK;
    }
};
CoCreatableClass(NotificationActivator);

void sdl_native_display_notification(const WhistNotification* notif) {
    HRESULT hr;

    // Create notification XML from template as UTF-8.
    std::string toast_xml_utf8 = std::format(
        "<toast><visual><binding template='ToastGeneric'>"
        "<text>{}</text><text>{}</text>"
        "</binding></visual></toast>",
        notif->title, notif->message);

    // Convert title and message to Windows UTF-16 strings.
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
    std::wstring toast_xml_utf16 = conv.from_bytes(toast_xml_utf8);

    // Create an XML document instance for the toast.
    ComPtr<ABI::Windows::Data::Xml::Dom::IXmlDocument> doc;
    hr = DesktopNotificationManagerCompat::CreateXmlDocumentFromString(toast_xml_utf16.c_str(),
                                                                       &doc);
    if (FAILED(hr)) {
        LOG_WARNING("Failed to create toast template XML: %#x.", hr);
        return;
    }

    // Get the toast notifier instance.
    ComPtr<ABI::Windows::UI::Notifications::IToastNotifier> notifier;
    hr = DesktopNotificationManagerCompat::CreateToastNotifier(&notifier);
    if (FAILED(hr)) {
        LOG_WARNING("Failed to create toast notifier: %#x.", hr);
        return;
    }

    // Make a toast notification out of the XML document.
    ComPtr<IToastNotification> toast;
    hr = DesktopNotificationManagerCompat::CreateToastNotification(doc.Get(), &toast);
    if (FAILED(hr)) {
        LOG_WARNING("Failed to create toast notification: %#x.", hr);
        return;
    }

    // Actually show the notification.
    hr = notifier->Show(toast.Get());
    if (FAILED(hr)) {
        LOG_WARNING("Failed to show toast: %#x.", hr);
        return;
    }

    LOG_INFO("Successfully showing toast notification!");
}

static const wchar_t* const path_start_menu =
    L"Microsoft\\Windows\\Start Menu\\Programs\\Whist Client.lnk";

static HRESULT register_shortcut(void) {
    HRESULT hr;

    PWSTR path_appdata;
    hr = ::SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &path_appdata);
    if (FAILED(hr)) {
        LOG_WARNING("Failed to get AppData path: %#x.", hr);
        return hr;
    }

    wchar_t path_shortcut[MAX_PATH];
    hr = ::PathCchCombine(path_shortcut, ARRAYSIZE(path_shortcut), path_appdata, path_start_menu);
    if (FAILED(hr)) {
        LOG_WARNING("Failed to create shortcut path: %#x.", hr);
        return hr;
    }

    DWORD attr = ::GetFileAttributesW(path_shortcut);
    if (attr != INVALID_FILE_ATTRIBUTES) {
        // Shortcut already exists, hopefully nothing to do.
        return S_OK;
    }

    wchar_t path_executable[MAX_PATH];
    DWORD len = ::GetModuleFileNameW(NULL, path_executable, MAX_PATH);
    if (len == 0) {
        DWORD err = GetLastError();
        LOG_WARNING("Failed to get executable path: %#x.", err);
        return HRESULT_FROM_WIN32(err);
    }

    ComPtr<IShellLink> shell_link;
    hr = ::CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
                            IID_PPV_ARGS(&shell_link));
    if (FAILED(hr)) {
        LOG_WARNING("Failed to create shell link instance: %#x.", hr);
        return hr;
    }

    hr = shell_link->SetPath(path_executable);
    if (FAILED(hr)) {
        LOG_WARNING("Failed to set executable path on shell link: %#x.", hr);
        return hr;
    }

    ComPtr<IPropertyStore> property_store;
    hr = shell_link.As(&property_store);
    if (FAILED(hr)) {
        LOG_WARNING("Failed to fetch shell link property store: %#x.", hr);
        return hr;
    }

    PROPVARIANT aumid_prop = {
        .vt = VT_LPWSTR,
        .pwszVal = (LPWSTR)whist_client_aumid,
    };
    hr = property_store->SetValue(PKEY_AppUserModel_ID, aumid_prop);
    if (FAILED(hr)) {
        LOG_WARNING("Failed to set AUMID property: %#x.", hr);
        return hr;
    }

    PROPVARIANT toast_activator_prop = {
        .vt = VT_CLSID,
        .puuid = const_cast<CLSID*>(&__uuidof(NotificationActivator)),
    };
    hr = property_store->SetValue(PKEY_AppUserModel_ToastActivatorCLSID, toast_activator_prop);
    if (FAILED(hr)) {
        LOG_WARNING("Failed to set toast activator property: %#x.", hr);
        return hr;
    }

    hr = property_store->Commit();
    if (FAILED(hr)) {
        LOG_WARNING("Failed to commit shell link property store: %#x.", hr);
        return hr;
    }

    ComPtr<IPersistFile> persist_file;
    hr = shell_link.As(&persist_file);
    if (FAILED(hr)) {
        LOG_WARNING("Failed to make persistent file link: %#x.", hr);
        return hr;
    }

    hr = persist_file->Save(path_shortcut, TRUE);
    if (FAILED(hr)) {
        LOG_WARNING("Failed to save persistent file link: %#x.", hr);
        return hr;
    }

    return S_OK;
}

void sdl_native_init_notifications(WhistFrontend* frontend) {
    SDLFrontendContext* context = (SDLFrontendContext*)frontend->context;
    HRESULT hr;

    hr = ::CoInitialize(NULL);
    if (FAILED(hr)) {
        LOG_WARNING("COM initialisation failed: %#x.", hr);
    }

    if (start_menu_entry) {
        hr = register_shortcut();
        if (FAILED(hr)) {
            LOG_WARNING("Shortcut failed to register: %#x.", hr);
        }
    }

    hr = DesktopNotificationManagerCompat::RegisterAumidAndComServer(
        whist_client_aumid, __uuidof(NotificationActivator));
    if (FAILED(hr)) {
        LOG_WARNING("Failed to register notification server.");
    } else {
        hr = DesktopNotificationManagerCompat::RegisterActivator();
        if (FAILED(hr)) {
            LOG_WARNING("Failed to register notification activator.");
        }
    }

    event_context = context;
}
