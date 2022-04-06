export enum InstalledBrowser {
  BRAVE = "Brave",
  OPERA = "Opera",
  CHROME = "Chrome",
  FIREFOX = "Firefox",
  CHROMIUM = "Chromium",
  EDGE = "Edge",
}

export const DEFAULT_ENCRYPTION_KEY = "peanuts"

// File paths for each browser
export const ChromeLinuxDefaultDir = [
  "~/.config/google-chrome/Default/",
  "~/.config/google-chrome-beta/Default/",
]
export const ChromeOSXDefaultDir = [
  "~/Library/Application Support/Google/Chrome/Default/",
]
export const ChromeWindowsDefaultDir = [
  "%LOCALAPPDATA%\\Google\\Chrome\\User Data\\Default\\Network",
  "%APPDATA%\\Google\\Chrome\\User Data\\Default\\Network",
]
export const ChromeWindowsKeys = [
  "%LOCALAPPDATA%\\Google\\Chrome\\User Data\\Local State",
  "%APPDATA%\\Google\\Chrome\\User Data\\Local State",
]

export const ChromiumLinuxDefaultDir = ["~/.config/chromium/Default/"]
export const ChromiumOSXDefaultDir = [
  "~/Library/Application Support/Chromium/Default/",
]
export const ChromiumWindowsKeys = [
  "%LOCALAPPDATA%\\Chromium\\User Data\\Local State",
  "%APPDATA%\\Chromium\\User Data\\Local State",
]

export const OperaLinuxDefaultDir = ["~/.config/opera/"]
export const OperaOSXDefaultDir = [
  "~/Library/Application Support/com.operasoftware.Opera/",
]
export const OperaWindowsKeys = [
  "%LOCALAPPDATA%\\Opera Software\\Opera Stable\\Local State",
  "%APPDATA%\\Opera Software\\Opera Stable\\Local State",
]

export const BraveLinuxDefaultDir = [
  "~/.config/BraveSoftware/Brave-Browser/Default/",
  "~/.config/BraveSoftware/Brave-Browser-Beta/Default/",
]
export const BraveOSXDefaultDir = [
  "~/Library/Application Support/BraveSoftware/Brave-Browser/Default/",
  "~/Library/Application Support/BraveSoftware/Brave-Browser-Beta/Default/",
]
export const BraveWindowsDefaultDir = [
  "%LOCALAPPDATA%\\BraveSoftware\\Brave-Browser\\User Data\\Default\\Network",
  "%APPDATA%\\BraveSoftware\\Brave-Browser\\User Data\\Default\\Network",
]

export const BraveWindowsKeys = [
  "%LOCALAPPDATA%\\BraveSoftware\\Brave-Browser\\User Data\\Local State",
  "%LOCALAPPDATA%\\BraveSoftware\\Brave-Browser-Beta\\User Data\\Local State",
  "%LOCALAPPDATA%\\BraveSoftware\\Brave-Browser-Betar\\User Data\\Local State",
  "%APPDATA%\\BraveSoftware\\Brave-Browser\\User Data\\Local State",
  "%APPDATA%\\BraveSoftware\\Brave-Browser-Beta\\User Data\\Local State",
  "%APPDATA%\\BraveSoftware\\Brave-Browser-Betar\\User Data\\Local State",
]

export const EdgeLinuxDefaultDir = [
  "~/.config/microsoft-edge/Default/",
  "~/.config/microsoft-edge-dev/Default/",
]
export const EdgeOSXDefaultDir = [
  "~/Library/Application Support/Microsoft Edge/Default/",
]
export const EdgeWindowsKeys = [
  "%APPDATA%\\Microsoft\\Edge\\User Data\\Local State",
  "%LOCALAPPDATA%\\Microsoft\\Edge\\User Data\\Local State",
]

// Linux DBus / Secret Session
export const BusSecretName = "org.freedesktop.secrets"
export const BusSecretPath = "/org/freedesktop/secrets"
export const SecretServiceName = "org.freedesktop.Secret.Service"

export const ALGORITHM_PLAIN = "plain"
// add later: const ALGORITHM_DH = 'dh-ietf1024-sha256-aes128-cbc-pkcs7'
