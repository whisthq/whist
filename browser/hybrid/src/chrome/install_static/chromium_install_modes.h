// Brand-specific types and constants for Whist.

#ifndef WHIST_BROWSER_HYBRID_SRC_CHROME_INSTALL_STATIC_CHROMIUM_INSTALL_MODES_H_
#define WHIST_BROWSER_HYBRID_SRC_CHROME_INSTALL_STATIC_CHROMIUM_INSTALL_MODES_H_

namespace install_static {

// Note: This list of indices must be kept in sync with the brand-specific
// resource strings in
// chrome/installer/util/prebuild/create_installer_string_rc.
enum InstallConstantIndex {
#if defined(OFFICIAL_BUILD)
  STABLE_INDEX,
  BETA_INDEX,
  NIGHTLY_INDEX,
#else
  DEVELOPER_INDEX,
#endif
  NUM_INSTALL_MODES,
};

}  // namespace install_static

#endif  // WHIST_BROWSER_HYBRID_SRC_CHROME_INSTALL_STATIC_CHROMIUM_INSTALL_MODES_H_
