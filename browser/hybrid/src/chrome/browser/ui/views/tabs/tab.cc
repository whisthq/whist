// Copyright 2022 Whist Technologies, Inc. All rights reserved.

#include "chrome/common/url_constants.h"

#include "chrome/browser/ui/views/tabs/tab.h"
#include "src/brave/chromium_src/chrome/browser/ui/views/tabs/tab.cc"

bool Tab::IsCloudTab() const {
  GURL url = data_.visible_url;
  return url.SchemeIs(content::kCloudScheme);
}
