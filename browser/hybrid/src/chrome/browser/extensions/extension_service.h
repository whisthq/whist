// Copyright 2022 Whist Technologies, Inc. All rights reserved.
#ifndef WHIST_BROWSER_HYBRID_SRC_CHROME_BROWSER_EXTENSIONS_EXTENSION_SERVICE_H_
#define WHIST_BROWSER_HYBRID_SRC_CHROME_BROWSER_EXTENSIONS_EXTENSION_SERVICE_H_

#define BraveExtensionService         \
  BraveExtensionService;              \
  friend class WhistExtensionService;
#include "src/brave/chromium_src/chrome/browser/extensions/extension_service.h"
#undef BraveExtensionService

#endif  // WHIST_BROWSER_HYBRID_SRC_CHROME_BROWSER_EXTENSIONS_EXTENSION_SERVICE_H_
