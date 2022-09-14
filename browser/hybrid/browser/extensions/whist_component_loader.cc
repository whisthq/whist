#include "whist_component_loader.h"

namespace extensions {
  WhistComponentLoader::WhistComponentLoader(ExtensionSystem* extension_system,
                                            Profile* profile)
      : BraveComponentLoader(extension_system, profile) {}

  WhistComponentLoader::~WhistComponentLoader() {}

  void WhistComponentLoader::AddWhistExtension() {
    if (!Exists(whist_extension_id)) {
      base::FilePath whist_extension_path(FILE_PATH_LITERAL(""));
      whist_extension_path = whist_extension_path.Append(FILE_PATH_LITERAL("whist_extension"));
      Add(IDR_WHIST_EXTENSION, whist_extension_path);
    }
  }

  void WhistComponentLoader::AddDefaultComponentExtensions(bool skip_session_components) {
    BraveComponentLoader::AddDefaultComponentExtensions(skip_session_components);
    AddWhistExtension();
  }
}  // namespace extensions
