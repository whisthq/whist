// Copyright 2022 Whist Technologies, Inc. All rights reserved.
#ifndef WHIST_BROWSER_HYBRID_BROWSER_UI_WEBUI_WHIST_WEB_UI_CONTROLLER_FACTORY_H_
#define WHIST_BROWSER_HYBRID_BROWSER_UI_WEBUI_WHIST_WEB_UI_CONTROLLER_FACTORY_H_

#include "base/memory/ptr_util.h"
#include "brave/browser/ui/webui/brave_web_ui_controller_factory.h"

class WhistWebUIControllerFactory : public BraveWebUIControllerFactory {
 public:
  WhistWebUIControllerFactory(const WhistWebUIControllerFactory&) = delete;
  WhistWebUIControllerFactory& operator=(const WhistWebUIControllerFactory&) =
      delete;

  content::WebUI::TypeID GetWebUIType(content::BrowserContext* browser_context,
                                      const GURL& url) override;
  std::unique_ptr<content::WebUIController> CreateWebUIControllerForURL(
      content::WebUI* web_ui,
      const GURL& url) override;

  static WhistWebUIControllerFactory* GetInstance();

 protected:
  friend struct base::DefaultSingletonTraits<WhistWebUIControllerFactory>;

  WhistWebUIControllerFactory();
  ~WhistWebUIControllerFactory() override;
};

#endif  // WHIST_BROWSER_HYBRID_BROWSER_UI_WEBUI_WHIST_WEB_UI_CONTROLLER_FACTORY_H_
