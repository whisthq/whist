// Brand-specific constants and install modes for Whist.

#include <stdlib.h>

#include "chrome/common/chrome_icon_resources_win.h"
#include "chrome/app/chrome_dll_resource.h"
#include "chrome/install_static/install_modes.h"

namespace install_static {

const wchar_t kCompanyPathName[] = L"Whist";

#if defined(OFFICIAL_BUILD)
const wchar_t kProductPathName[] = L"Whist-Browser";
#else
const wchar_t kProductPathName[] = L"Whist-Browser-Development";
#endif

const size_t kProductPathNameLength = _countof(kProductPathName) - 1;

const char kSafeBrowsingName[] = "chromium";

const char kDeviceManagementServerHostName[] = "";

#if defined(OFFICIAL_BUILD)
// Regarding to install switch, use same value in
// chrome/installer/mini_installer/configuration.cc
const InstallConstants kInstallModes[] = {
    // The primary install mode for stable Whist.
    {
        sizeof(kInstallModes[0]),
        STABLE_INDEX,  // The first mode is for stable/beta/dev.
        "",            // No install switch for the primary install mode.
        L"",           // Empty install_suffix for the primary install mode.
        L"",           // No logo suffix for the primary install mode.
        L"{08F1CB9D-5D59-4554-9DDB-30DBEA15C6B0}",
        L"Whist",                                   // A distinct base_app_name.
        L"Whist",                                   // A distinct base_app_id.
        L"WhistHTML",                               // ProgID prefix.
        L"Whist HTML Document",                     // ProgID description.
        L"{08F1CB9D-5D59-4554-9DDB-30DBEA15C6B0}",  // Active Setup GUID.
        L"{478F0D9C-5AC1-42D9-BAAF-613D2D0EC81D}",  // CommandExecuteImpl CLSID.
        {0xceb3bf5f,
         0xd27d,
         0x4375,
         {0xbc, 0xdf, 0x39, 0x52, 0xde, 0x90, 0x23,
          0xdc}},  // Toast activator CLSID.
	{0xf980b4c4,
	 0x92f5,
	 0x4253,
	 {0xab, 0x41, 0x14, 0x6c, 0xeb, 0x27, 0x22, 0xc8}},  // Elevator CLSID.
	{0x67743c38,
	 0x78d4,
	 0x4bda,
	 {0xb3, 0xf6, 0x3a, 0x3, 0x11, 0xa, 0x86, 0xc1}},
        L"",  // The empty string means "stable".
        ChannelStrategy::FLOATING,
        true,  // Supports system-level installs.
        true,  // Supports in-product set as default browser UX.
        true,  // Supports retention experiments.
        icon_resources::kApplicationIndex,  // App icon resource index.
        IDR_MAINFRAME,                      // App icon resource id.
        L"S-1-15-2-3251537155-1984446955-2931258699-841473695-1938553385-"
        L"944012149-",  // App container sid prefix for sandbox.
    },
    // A secondary install mode for Whist Beta
    {
        sizeof(kInstallModes[0]),
        BETA_INDEX,     // The mode for the side-by-side beta channel.
        "chrome-beta",  // Install switch.
        L"-Beta",       // Install suffix.
        L"Beta",        // Logo suffix.
        L"{82837D21-3FE0-4AC9-BC60-342114D5F84F}",  // A distinct app GUID.
        L"Whist Beta",                              // A distinct base_app_name.
        L"WhistBeta",                               // A distinct base_app_id.
        L"WhistBHTML",                              // ProgID prefix.
        L"Whist Beta HTML Document",                // ProgID description.
        L"{82837D21-3FE0-4AC9-BC60-342114D5F84F}",  // Active Setup GUID.
        L"",                                        // CommandExecuteImpl CLSID.
	{0x8ada910a,
	 0xbedd,
	 0x4369,
	 {0x8b, 0x10, 0x8, 0xee, 0xb1, 0x8c, 0xcb,
	  0x81}},  // Toast activator CLSID.
	{0x884700cc,
	 0xcfb4,
	 0x43a1,
	 {0x8f, 0xec, 0x93, 0x71, 0xd4, 0x65, 0xa5, 0xee}},  // Elevator CLSID.
	{0x75727934,
	 0x866d,
	 0x46de,
	 {0x9a, 0x4f, 0xe3, 0x27, 0x57, 0x68, 0x6a, 0x5c}},
        L"beta",  // Forced channel name.
        ChannelStrategy::FIXED,
        true,  // Supports system-level installs.
        true,  // Supports in-product set as default browser UX.
        true,  // Supports retention experiments.
        icon_resources::kBetaApplicationIndex,  // App icon resource index.
        IDR_X005_BETA,                          // App icon resource id.
        L"S-1-15-2-3251537155-1984446955-2931258699-841473695-1938553385-"
        L"944012150-",  // App container sid prefix for sandbox.
    },
    // A secondary install mode for Whist SxS (canary).
    {
        sizeof(kInstallModes[0]),
        NIGHTLY_INDEX,  // The mode for the side-by-side nightly channel.
        "chrome-sxs",   // Install switch.
        L"-Nightly",    // Install suffix.
        L"Canary",      // Logo suffix.
        L"{C0277CCB-F2FD-4DB9-8F7A-E05DA660E0EC}",  // A distinct app GUID.
        L"Whist Nightly",                           // A distinct base_app_name.
        L"WhistNightly",                            // A distinct base_app_id.
        L"WhistSSHTM",                              // ProgID prefix.
        L"Whist Nightly HTML Document",             // ProgID description.
        L"{C0277CCB-F2FD-4DB9-8F7A-E05DA660E0EC}",  // Active Setup GUID.
        L"{F3D369CE-A3DF-48EC-ABB0-4A8AB4E0955F}",  // CommandExecuteImpl CLSID.
	{0x148f848a,
	 0xdb6b,
	 0x4458,
	 {0x82, 0x77, 0x25, 0x5b, 0xfd, 0x22, 0x34,
	  0xe6}},  // Toast activator CLSID.
	{0x9e8afc20,
	 0xdbb8,
	 0x49fc,
	 {0xa4, 0xf4, 0xbb, 0xe3, 0xd8, 0xb0, 0xc9, 0x6}},  // Elevator CLSID.
	{0x3aaf1505,
	 0x1022,
	 0x456f,
	 {0x82, 0xf0, 0x5b, 0xc8, 0x71, 0x2f, 0xe0, 0xaa}},
        L"nightly",  // Forced channel name.
        ChannelStrategy::FIXED,
        true,  // Support system-level installs.
        true,  // Support in-product set as default browser UX.
        true,  // Supports retention experiments.
        icon_resources::kSxSApplicationIndex,  // App icon resource index.
        IDR_SXS,                               // App icon resource id.
        L"S-1-15-2-3251537155-1984446955-2931258699-841473695-1938553385-"
        L"944012152-",  // App container sid prefix for sandbox.
    },
};
#else
const InstallConstants kInstallModes[] = {
    // The primary (and only) install mode for Whist developer build.
    {
        sizeof(kInstallModes[0]),
        DEVELOPER_INDEX,  // The one and only mode for developer mode.
        "",               // No install switch for the primary install mode.
        L"",              // Empty install_suffix for the primary install mode.
        L"",              // No logo suffix for the primary install mode.
        L"",            // Empty app_guid since no integraion with Brave Update.
        L"Whist Development",  // A distinct base_app_name.
        L"WhistDevelopment",   // A distinct base_app_id.
        L"WhistDevHTM",                             // ProgID prefix.
        L"Whist Development HTML Document",           // ProgID description.
	L"{A0C9C730-FBC4-47B0-A24E-9594D384F1E4}",  // Active Setup GUID.
	L"{4E874C52-D87D-4FB8-A615-9C72526A89BB}",  // CommandExecuteImpl CLSID.
	{ 0x425c34af,
	  0xca22,
	  0x4249,
	  { 0xb6, 0x33, 0x67, 0x80, 0x1a, 0x77, 0x7b,
	  0x74 } },  // Toast activator CLSID.
	{ 0xa82eea58,
	  0xc4d0,
	  0x4d40,
	  { 0xb4, 0x76, 0xca, 0x35, 0x1f, 0x6d, 0x6d,
	    0x6c } },  // Elevator CLSID.
	{ 0x9aae034d, 0x61d4, 0x4f99,
	  { 0x84, 0xdc, 0x10, 0x90, 0x6a, 0x4f, 0xfc, 0x67 } },
        L"",    // Empty default channel name since no update integration.
        ChannelStrategy::UNSUPPORTED,
        true,   // Supports system-level installs.
        true,   // Supports in-product set as default browser UX.
        false,  // Does not support retention experiments.
        icon_resources::kApplicationIndex,  // App icon resource index.
        IDR_MAINFRAME,                      // App icon resource id.
        L"S-1-15-2-3251537155-1984446955-2931258699-841473695-1938553385-"
        L"944012148-",  // App container sid prefix for sandbox.
    },
};
#endif

static_assert(_countof(kInstallModes) == NUM_INSTALL_MODES,
              "Imbalance between kInstallModes and InstallConstantIndex");

}  // namespace install_static
