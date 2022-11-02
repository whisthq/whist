// chrome_appid.cc

#include "chrome/installer/mini_installer/appid.h"

namespace google_update {

#if defined(OFFICIAL_BUILD)
const wchar_t kAppGuid[] = L"{08F1CB9D-5D59-4554-9DDB-30DBEA15C6B0}";
const wchar_t kMultiInstallAppGuid[] =
    L"{06EF0039-F713-4CCB-A413-E984C86A78A0}";
const wchar_t kBetaAppGuid[] = L"{82837D21-3FE0-4AC9-BC60-342114D5F84F}";
const wchar_t kSxSAppGuid[] = L"{C0277CCB-F2FD-4DB9-8F7A-E05DA660E0EC}";
#else
const wchar_t kAppGuid[] = L"";
const wchar_t kMultiInstallAppGuid[] = L"";
#endif

}  // namespace google_update
