export enum InstalledBrowser {
  BRAVE = "Brave",
  OPERA = "Opera",
  CHROME = "Chrome",
  FIREFOX = "Firefox",
  CHROMIUM = "Chromium",
  EDGE = "Edge",
}

// File paths for each browser
export const ChromeLinuxDefaultDir = [
  "~/.config/google-chrome/Default/",
  "~/.config/google-chrome-beta/Default/",
]
export const ChromeOSXDefaultDir = [
  "~/Library/Application Support/Google/Chrome/Default/",
]
export const ChromeWindowsDefaultDir = [
  "%LocalAppData%\\Google\\Chrome\\User Data/Default/",
]
export const ChromeWindowsKeys = [
  { env: "APPDATA", path: "..\\Local\\Google\\Chrome\\User Data\\Local State" },
  { env: "LOCALAPPDATA", path: "Google\\Chrome\\User Data\\Local State" },
  { env: "APPDATA", path: "Google\\Chrome\\User Data\\Local State" },
]

export const ChromiumLinuxDefaultDir = ["~/.config/chromium/Default/"]
export const ChromiumOSXDefaultDir = [
  "~/Library/Application Support/Chromium/Default/",
]
export const ChromiumWindowsKeys = [
  { env: "APPDATA", path: "..\\Local\\Chromium\\User Data\\Local State" },
  { env: "LOCALAPPDATA", path: "Chromium\\User Data\\Local State" },
  { env: "APPDATA", path: "Chromium\\User Data\\Local State" },
]

export const OperaLinuxDefaultDir = ["~/.config/opera/"]
export const OperaOSXDefaultDir = [
  "~/Library/Application Support/com.operasoftware.Opera/",
]
export const OperaWindowsKeys = [
  {
    env: "APPDATA",
    path: "..\\Local\\Opera Software\\Opera Stable\\Local State",
  },
  { env: "LOCALAPPDATA", path: "Opera Software\\Opera Stable\\Local State" },
  { env: "APPDATA", path: "Opera Software\\Opera Stable\\Local State" },
]

export const BraveLinuxDefaultDir = [
  "~/.config/BraveSoftware/Brave-Browser/Default/",
  "~/.config/BraveSoftware/Brave-Browser-Beta/Default/",
]
export const BraveOSXDefaultDir = [
  "~/Library/Application Support/BraveSoftware/Brave-Browser/Default/",
  "~/Library/Application Support/BraveSoftware/Brave-Browser-Beta/Default/",
]
export const BraveWindowsKeys = [
  {
    env: "APPDATA",
    path: "..\\Local\\BraveSoftware\\Brave-Browser\\User Data\\Local State",
  },
  {
    env: "LOCALAPPDATA",
    path: "BraveSoftware\\Brave-Browser\\User Data\\Local State",
  },
  {
    env: "APPDATA",
    path: "BraveSoftware\\Brave-Browser\\User Data\\Local State",
  },
  {
    env: "APPDATA",
    path: "..\\Local\\BraveSoftware\\Brave-Browser-Beta\\User Data\\Local State",
  },
  {
    env: "LOCALAPPDATA",
    path: "BraveSoftware\\Brave-Browse-Betar\\User Data\\Local State",
  },
  {
    env: "APPDATA",
    path: "BraveSoftware\\Brave-Browser-Beta\\User Data\\Local State",
  },
]

export const EdgeLinuxDefaultDir = [
  "~/.config/microsoft-edge/Default/",
  "~/.config/microsoft-edge-dev/Default/",
]
export const EdgeOSXDefaultDir = [
  "~/Library/Application Support/Microsoft Edge/Default/",
]
export const EdgeWindowsKeys = [
  {
    env: "APPDATA",
    path: "..\\Local\\Microsoft\\Edge\\User Data\\Local State",
  },
  { env: "LOCALAPPDATA", path: "Microsoft\\Edge\\User Data\\Local State" },
  { env: "APPDATA", path: "Microsoft\\Edge\\User Data\\Local State" },
]

// Linux DBus / Secret Session
export const BusSecretName = "org.freedesktop.secrets"
export const BusSecretPath = "/org/freedesktop/secrets"
export const SecretServiceName = "org.freedesktop.Secret.Service"

export const ALGORITHM_PLAIN = "plain"
// add later: const ALGORITHM_DH = 'dh-ietf1024-sha256-aes128-cbc-pkcs7'
