// The Electron BrowserWindow API can be passed a hash parameter as data.
// We use this so that renderer threads can decide which view component to
// render as soon as a window appears.
export const WindowHashAuth = "AUTH"
export const WindowHashSignout = "SIGNOUT"
export const WindowHashPayment = "PAYMENT"
export const WindowHashProtocol = "PROTOCOL"
export const WindowHashExitTypeform = "EXIT_TYPEFORM"
export const WindowHashBugTypeform = "BUG_TYPEFORM"
export const WindowHashOnboarding = "ONBOARDING"
export const WindowHashSleep = "SLEEP"
export const WindowHashUpdate = "UPDATE"
export const WindowHashImport = "IMPORTER"
export const WindowHashLoading = "LOADING"
export const WindowHashOmnibar = "OMNIBAR"
