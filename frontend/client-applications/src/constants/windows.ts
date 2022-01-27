// The Electron BrowserWindow API can be passed a hash parameter as data.
// We use this so that renderer threads can decide which view component to
// render as soon as a window appears.
export const WindowHashAuth = "AUTH"
export const WindowHashSignout = "SIGNOUT"
export const WindowHashPayment = "PAYMENT"
export const WindowHashProtocol = "PROTOCOL"
export const WindowHashOnboarding = "ONBOARDING"
export const WindowHashSleep = "SLEEP"
export const WindowHashUpdate = "UPDATE"
export const WindowHashImport = "IMPORTER"
export const WindowHashLaunchLoading = "LAUNCH_LOADING"
export const WindowHashImportLoading = "IMPORT_LOADING"
export const WindowHashOmnibar = "OMNIBAR"
export const WindowHashSpeedtest = "SPEEDTEST"
export const WindowHashLicense = "LICENSE"
export const WindowHashWelcome = "WELCOME"
export const WindowHashSupport = "SUPPORT"

export const width = {
  xs: { width: 16 * 24 },
  sm: { width: 16 * 32 },
  md: { width: 16 * 40 },
  lg: { width: 16 * 48 },
  xl: { width: 16 * 56 },
}

export const height = {
  xs: { height: 16 * 8 },
  sm: { height: 16 * 20 },
  md: { height: 16 * 32 },
  lg: { height: 16 * 44 },
  xl: { height: 16 * 56 },
}
