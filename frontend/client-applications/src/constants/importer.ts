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
export const ChromiumLinuxDefaultDir = ["~/.config/chromium/Default/"]
export const ChromiumOSXDefaultDir = [
  "~/Library/Application Support/Chromium/Default/",
]
export const OperaLinuxDefaultDir = ["~/.config/opera/"]
export const OperaOSXDefaultDir = [
  "~/Library/Application Support/com.operasoftware.Opera/",
]
export const BraveLinuxDefaultDir = [
  "~/.config/BraveSoftware/Brave-Browser/Default/",
  "~/.config/BraveSoftware/Brave-Browser-Beta/Default/",
]
export const BraveOSXDefaultDir = [
  "~/Library/Application Support/BraveSoftware/Brave-Browser/Default/",
  "~/Library/Application Support/BraveSoftware/Brave-Browser-Beta/Default/",
]
export const EdgeLinuxDefaultDir = [
  "~/.config/microsoft-edge/Default/",
  "~/.config/microsoft-edge-dev/Default/",
]
export const EdgeOSXDefaultDir = [
  "~/Library/Application Support/Microsoft Edge/Default/",
]

// Linux DBus / Secret Session
export const BusSecretName = "org.freedesktop.secrets"
export const BusSecretPath = "/org/freedesktop/secrets"
export const SecretServiceName = "org.freedesktop.Secret.Service"

export const ALGORITHM_PLAIN = "plain"
// add later: const ALGORITHM_DH = 'dh-ietf1024-sha256-aes128-cbc-pkcs7'
