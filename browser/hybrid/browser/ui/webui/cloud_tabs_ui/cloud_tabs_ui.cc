// Copyright 2022 Whist Technologies, Inc. All rights reserved.

#include "cloud_tabs_ui.h"

#include <utility>

#include "base/rand_util.h"
#include "brave/common/pref_names.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/common/url_constants.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"
#include "components/grit/brave_components_resources.h"

#include "whist/browser/hybrid/components/cloud_tabs_ui/resources/grit/cloud_tabs_ui_generated_map.h"

using content::WebUIMessageHandler;

namespace {
class CloudUIHandler : public WebUIMessageHandler {
 public:
  CloudUIHandler() {}
  CloudUIHandler(const CloudUIHandler&) = delete;
  CloudUIHandler& operator=(const CloudUIHandler&) = delete;

  // WebUIMessageHandler implementation.
  void RegisterMessages() override;

  // Added by Whist
 private:
  void OnWhistMandelboxInfoChanged();

  PrefChangeRegistrar local_state_change_registrar_;
  PrefChangeRegistrar profile_state_change_registrar_;
};

// Added by Whist
void CloudUIHandler::RegisterMessages() {}
}

std::string RandomString() {
  std::string random_string;
  for (int index = 0; index < 32; ++index) {
    random_string.append(1, static_cast<char>(base::RandInt('A', 'Z')));
  }
  return random_string;
}

WhistCloudUI::WhistCloudUI(content::WebUI* web_ui, const GURL& url)
    : content::WebUIController(web_ui) {
  content::WebUIDataSource* source = CreateAndAddWebUIDataSource(
      web_ui, content::kCloudScheme, kCloudTabsUiGenerated, kCloudTabsUiGeneratedSize,
      IDR_WHIST_CLOUD_TABS_UI_HTML,
      /*disable_trusted_types_csp=*/true);

  web_ui->AddMessageHandler(std::make_unique<CloudUIHandler>());

  // Generate random nonce and set security policy accordingly
  std::string random_nonce = RandomString();
  source->OverrideContentSecurityPolicy(
    network::mojom::CSPDirectiveName::ScriptSrc,
    "script-src cloud: chrome://resources 'nonce-" + random_nonce + "' 'self';");
  source->AddString("whistUINonce", random_nonce);
  // Required resources.
  source->UseStringsJs();
  source->SetDefaultResource(IDR_WHIST_CLOUD_TABS_UI_HTML);

  Profile* profile = Profile::FromWebUI(web_ui);
  content::WebUIDataSource::Add(profile, source);
}

WhistCloudUI::~WhistCloudUI() = default;
