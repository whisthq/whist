#include "chrome/common/chrome_constants.h"

#include "build/build_config.h"
#include "chrome/common/chrome_version.h"

#if BUILDFLAG(IS_POSIX) | BUILDFLAG(IS_WIN)
#define kBrowserProcessExecutableName kBrowserProcessExecutableName_Unused
#define kBrowserProcessExecutablePath kBrowProcessExecutableName_Unused
#define kHelperProcessExecutablePath kHelperProcessExecutablePath_Unused
#endif  // BUILDFLAG(IS_POSIX) | BUILDFLAG(IS_WIN)

#if BUILDFLAG(IS_WIN)
#define kHelperProcessExecutableName kHelperProcessExecutableName_Unused
#endif  // BUILDFLAG(IS_WIN)

#include "chrome/common/chrome_constants.cc"

#if BUILDFLAG(IS_POSIX) | BUILDFLAG(IS_WIN)
#undef kBrowserProcessExecutableName
#undef kBrowserProcessExecutablePath
#undef kHelperProcessExecutablePath
#endif  // BUILDFLAG(IS_POSIX) | BUILDFLAG(IS_WIN)

#if BUILDFLAG(IS_WIN)
#undef kHelperProcessExecutableName
#endif  //BUILDFLAG(IS_WIN)

#define FPL FILE_PATH_LITERAL

#if BUILDFLAG(IS_MAC)
#define PRODUCT_STRING BRAVE_PRODUCT_STRING
#endif  // BUILDFLAG(IS_MAC)

namespace chrome {

#if BUILDFLAG(IS_WIN)
const base::FilePath::CharType kBrowserProcessExecutableName[] =
    FPL("whist.exe");
const base::FilePath::CharType kHelperProcessExecutableName[] =
    FPL("whist.exe");
#elif BUILDFLAG(IS_POSIX)
const base::FilePath::CharType kBrowserProcessExecutableName[] = FPL("whist");
#endif  // OS_*

#if BUILDFLAG(IS_WIN)
const base::FilePath::CharType kBrowserProcessExecutablePath[] =
    FPL("whist.exe");
const base::FilePath::CharType kHelperProcessExecutablePath[] =
    FPL("whist.exe");
#elif BUILDFLAG(IS_POSIX)
const base::FilePath::CharType kBrowserProcessExecutablePath[] = FPL("whist");
const base::FilePath::CharType kHelperProcessExecutablePath[] = FPL("whist");
#endif  // OS_*

}  // namespace chrome

#undef FPL
