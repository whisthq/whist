#ifndef WHIST_BROWSER_EXTENSIONS_WHIST_COMPONENT_LOADER_H
#define WHIST_BROWSER_EXTENSIONS_WHIST_COMPONENT_LOADER_H

#include "brave/components/whist_extension/grit/whist_extension.h"
#include "extensions/common/constants.h"
#include "src/brave/browser/extensions/brave_component_loader.h"

namespace extensions {
class WhistComponentLoader : public BraveComponentLoader {
 public:
  WhistComponentLoader(ExtensionSystem* extension_system,
                       Profile* browser_context);
  WhistComponentLoader(const WhistComponentLoader&) = delete;
  WhistComponentLoader& operator=(const WhistComponentLoader&) = delete;
  ~WhistComponentLoader() override;

  void AddDefaultComponentExtensions(bool skip_session_components) override;
  void AddWhistExtension();
};
}  // namespace extensions

#endif  // WHIST_BROWSER_EXTENSIONS_WHIST_COMPONENT_LOADER_H
