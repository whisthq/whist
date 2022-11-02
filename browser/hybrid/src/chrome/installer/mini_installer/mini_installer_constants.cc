#include "chrome/installer/mini_installer/mini_installer_constants.h"

#if defined(OFFICIAL_BUILD)
#define kClientsKeyBase kClientsKeyBase_Unused
#define kClientStateKeyBase kClientStateKeyBase_Unused
#define kCleanupRegistryKey kCleanupRegistryKey_Unused
#endif

#include "//chrome/installer/mini_installer/mini_installer_constants.cc"

#if defined(OFFICIAL_BUILD)
#undef kClientsKeyBase
#undef kClientStateKeyBase
#undef kCleanupRegistryKey

namespace mini_installer {

const wchar_t kClientsKeyBase[] = L"Software\\Whist\\Update\\Clients\\";
const wchar_t kClientStateKeyBase[] = L"Software\\Whist\\Update\\ClientState\\";
const wchar_t kCleanupRegistryKey[] =
  L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Whist";
  
}  // namespace mini_installer

#endif
