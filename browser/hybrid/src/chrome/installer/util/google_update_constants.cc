#include "chrome/installer/util/google_update_constants.h"

#define kChromeUpgradeCode kChromeUpgradeCode_Unused
#define kGoogleUpdateUpgradeCode kGoogleUpdateUpgradeCode_Unused
#define kGoogleUpdateSetupExe kGoogleUpdateSetupExe_Unused
#define kRegPathClients kRegPathClients_Unused
#define kRegPathClientState kRegPathClientState_Unused
#define kRegPathClientStateMedium kRegPathClientStateMedium_Unused
#define kRegPathGoogleUpdate kRegPathGoogleUpdate_Unused

#include "src/chrome/installer/util/google_update_constants.cc"

#undef kChromeUpgradeCode
#undef kGoogleUpdateUpgradeCode
#undef kGoogleUpdateSetupExe
#undef kRegPathClients
#undef kRegPathClientState
#undef kRegPathClientStateMedium
#undef kRegPathGoogleUpdate

namespace google_update {

const wchar_t kChromeUpgradeCode[] = L"{08F1CB9D-5D59-4554-9DDB-30DBEA15C6B0}";
const wchar_t kGoogleUpdateUpgradeCode[] =
    L"{6E20326D-D228-43F2-A18F-E48D24EC9E53}";
const wchar_t kGoogleUpdateSetupExe[] = L"WhistUpdateSetup.exe";
const wchar_t kRegPathClients[] = L"Software\\Whist\\Update\\Clients";
const wchar_t kRegPathClientState[] = L"Software\\Whist\\Update\\ClientState";
const wchar_t kRegPathClientStateMedium[] =
    L"Software\\Whist\\Update\\ClientStateMedium";
const wchar_t kRegPathGoogleUpdate[] = L"Software\\Whist\\Update";

}  // namespace google_update
