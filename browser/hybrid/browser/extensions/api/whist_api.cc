#include "whist_api.h"

#if defined(WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <memory>
#include <string>
#include <locale.h>

#include "base/environment.h"
#include "brave/common/extensions/api/whist.h"
#include "brave/common/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/browser.h"
#include "components/prefs/pref_service.h"
#include "extensions/browser/event_router.h"

#include "whist_api_native.h"

namespace extensions {
namespace api {

ExtensionFunction::ResponseAction WhistBroadcastWhistMessageFunction::Run() {
  std::unique_ptr<whist::BroadcastWhistMessage::Params> params(
      whist::BroadcastWhistMessage::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  Profile* profile = Profile::FromBrowserContext(browser_context());

  auto event = std::make_unique<extensions::Event>(
      extensions::events::WHIST_WEBUI_MESSAGE,
      api::whist::OnMessage::kEventName,
      api::whist::OnMessage::Create(params->message),
      profile);

  if (EventRouter* event_router = EventRouter::Get(profile))
    event_router->BroadcastEvent(std::move(event));

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction WhistGetKeyboardRepeatRateFunction::Run() {
  int repeat_rate = native_get_keyboard_repeat_rate();
  return RespondNow(OneArgument(base::Value(repeat_rate)));
}

ExtensionFunction::ResponseAction WhistGetKeyboardRepeatInitialDelayFunction::Run() {
  int initial_delay = native_get_keyboard_repeat_initial_delay();
  return RespondNow(OneArgument(base::Value(initial_delay)));
}

ExtensionFunction::ResponseAction WhistGetSystemLanguageFunction::Run() {
  char locale_str_buf[64];
  native_get_system_language(locale_str_buf, sizeof(locale_str_buf));
  std::string locale_str(locale_str_buf);
  // If no locale was obtained, try std::locale
  if (locale_str.size() == 0) {
    locale_str = std::locale("").name();
  }
  return RespondNow(OneArgument(base::Value(locale_str)));
}

ExtensionFunction::ResponseAction WhistGetUserLocaleFunction::Run() {
  base::Value::Dict locale_dict;

#if BUILDFLAG(IS_MAC)
  locale_dict.Set("LC_ALL", setlocale(LC_ALL, ""));
  locale_dict.Set("LC_COLLATE", setlocale(LC_COLLATE, ""));
  locale_dict.Set("LC_CTYPE", setlocale(LC_CTYPE, ""));
  locale_dict.Set("LC_MESSAGES", setlocale(LC_MESSAGES, ""));
  locale_dict.Set("LC_MONETARY", setlocale(LC_MONETARY, ""));
  locale_dict.Set("LC_NUMERIC", setlocale(LC_NUMERIC, ""));
  locale_dict.Set("LC_TIME", setlocale(LC_TIME, ""));
#endif

  return RespondNow(OneArgument(base::Value(std::move(locale_dict))));
}

}  // namespace api
}  // namespace extensions
