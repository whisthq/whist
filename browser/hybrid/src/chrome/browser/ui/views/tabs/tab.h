// Copyright 2022 Whist Technologies, Inc. All rights reserved.
#ifndef WHIST_BROWSER_HYBRID_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_H_
#define WHIST_BROWSER_HYBRID_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_H_


#define IsActive \
  IsCloudTab() const; \
  bool IsActive

#include "src/brave/chromium_src/chrome/browser/ui/views/tabs/tab.h"

#undef IsActive

#endif  // WHIST_BROWSER_HYBRID_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_H_
