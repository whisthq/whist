// Copyright 2022 Whist Technologies, Inc. All rights reserved.

#ifndef CHROME_BROWSER_UI_WEBUI_WHIST_CLOUD_WHIST_CLOUD_UI_H_
#define CHROME_BROWSER_UI_WEBUI_WHIST_CLOUD_WHIST_CLOUD_UI_H_

#include "content/public/browser/web_ui_controller.h"

// The WebUI for cloud:*.
class WhistCloudUI : public content::WebUIController {
 public:
  WhistCloudUI(content::WebUI* web_ui, const GURL& url);

  WhistCloudUI(const WhistCloudUI&) = delete;
  WhistCloudUI& operator=(const WhistCloudUI&) = delete;

  ~WhistCloudUI() override;
};

#endif  // CHROME_BROWSER_UI_WEBUI_WHIST_CLOUD_WHIST_CLOUD_UI_H_
