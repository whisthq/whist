#ifndef WHIST_HYBRID_SRC_CHROME_BROWSER_EXTENSIONS_EXTENSION_SERVICE_H
#define WHIST_HYBRID_SRC_CHROME_BROWSER_EXTENSIONS_EXTENSION_SERVICE_H

#define BraveExtensionService         \
  BraveExtensionService;              \
  friend class WhistExtensionService;
#include "src/brave/chromium_src/chrome/browser/extensions/extension_service.h"
#undef BraveExtensionService

#endif  // WHIST_HYBRID_SRC_CHROME_BROWSER_EXTENSIONS_EXTENSION_SERVICE_H
