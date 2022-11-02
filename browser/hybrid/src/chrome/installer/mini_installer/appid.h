// appid.h

#ifndef CHROME_INSTALLER_MINI_INSTALLER_APPID_H_
#define CHROME_INSTALLER_MINI_INSTALLER_APPID_H_

// The appid included by the mini_installer.
namespace google_update {

extern const wchar_t kAppGuid[];
extern const wchar_t kMultiInstallAppGuid[];

#if defined(OFFICIAL_BUILD)
extern const wchar_t kBetaAppGuid[];
extern const wchar_t kSxSAppGuid[];
#endif  // defined(OFFICIAL_BUILD)

}  // namespace google_update

#endif  // CHROME_INSTALLER_MINI_INSTALLER_APPID_H_
