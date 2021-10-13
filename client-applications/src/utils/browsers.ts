import {
  BraveLinuxCookieFiles,
  BraveOSXCookieFiles,
  ChromeLinuxCookieFiles,
  ChromeOSXCookieFiles,
  ChromiumLinuxCookieFiles,
  ChromiumOSXCookieFiles,
  EdgeLinuxCookieFiles,
  EdgeOSXCookieFiles,
  OperaLinuxCookieFiles,
  OperaOSXCookieFiles,
} from "./constants"

const getCookieFilePath = (browser: string): string[] => {
  switch (process.platform) {
    case "darwin": {
      switch (browser.toLowerCase()) {
        case "chrome": {
          return ChromeOSXCookieFiles
        }
        case "opera": {
          return OperaOSXCookieFiles
        }
        case "edge": {
          return EdgeOSXCookieFiles
        }
        case "chromium": {
          return ChromiumOSXCookieFiles
        }
        case "brave": {
          return BraveOSXCookieFiles
        }
      }

      break
    }
    case "linux": {
      switch (browser.toLowerCase()) {
        case "chrome": {
          return ChromeLinuxCookieFiles
        }
        case "opera": {
          return OperaLinuxCookieFiles
        }
        case "edge": {
          return EdgeLinuxCookieFiles
        }
        case "chromium": {
          return ChromiumLinuxCookieFiles
        }
        case "brave": {
          return BraveLinuxCookieFiles
        }
      }
      break
    }
  }

  return []
}
