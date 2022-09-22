#include "whist_web_ui_controller_factory.h"
#include <memory>
#include "base/logging.h"
#include "chrome/common/url_constants.h"
#include "whist/browser/hybrid/browser/ui/webui/cloud_tabs_ui/cloud_tabs_ui.h"

using content::WebUI;
using content::WebUIController;

namespace {
typedef WebUIController* (*WebUIFactoryFunction)(WebUI* web_ui,
                                                 const GURL& url);

WebUIController* NewWebUI(WebUI* web_ui, const GURL& url) {
  if (url.SchemeIs(content::kCloudScheme)) {
    return new WhistCloudUI(web_ui, url);
  }

  return nullptr;
}

WebUIFactoryFunction GetWebUIFactoryFunction(WebUI* web_ui, const GURL& url) {
  if (url.SchemeIs(content::kCloudScheme)) {
    return &NewWebUI;
  }

  return nullptr;
}
}  // namespace

WebUI::TypeID WhistWebUIControllerFactory::GetWebUIType(
    content::BrowserContext* browser_context,
    const GURL& url) {
  WebUIFactoryFunction function = GetWebUIFactoryFunction(NULL, url);
  if (function) {
    return reinterpret_cast<WebUI::TypeID>(function);
  }
  return BraveWebUIControllerFactory::GetWebUIType(browser_context, url);
}

std::unique_ptr<WebUIController>
WhistWebUIControllerFactory::CreateWebUIControllerForURL(WebUI* web_ui,
                                                         const GURL& url) {
  WebUIFactoryFunction function = GetWebUIFactoryFunction(web_ui, url);
  if (!function) {
    return BraveWebUIControllerFactory::CreateWebUIControllerForURL(web_ui,
                                                                    url);
  }

  return base::WrapUnique((*function)(web_ui, url));
}

// static
WhistWebUIControllerFactory* WhistWebUIControllerFactory::GetInstance() {
  return base::Singleton<WhistWebUIControllerFactory>::get();
}

WhistWebUIControllerFactory::WhistWebUIControllerFactory() {}
WhistWebUIControllerFactory::~WhistWebUIControllerFactory() {}
