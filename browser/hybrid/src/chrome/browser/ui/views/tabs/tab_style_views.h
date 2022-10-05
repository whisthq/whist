// Copyright 2022 Whist Technologies, Inc. All rights reserved.
#ifndef WHIST_BROWSER_HYBRID_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_STYLE_VIEWS_H_
#define WHIST_BROWSER_HYBRID_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_STYLE_VIEWS_H_

#define CreateForTab CreateForTab_ChromiumImpl(Tab* tab); \
  static std::unique_ptr<TabStyleViews> CreateForTab

#include "src/chrome/browser/ui/views/tabs/tab_style_views.h"
#undef CreateForTab

#endif  // WHIST_BROWSER_HYBRID_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_STYLE_VIEWS_H_
