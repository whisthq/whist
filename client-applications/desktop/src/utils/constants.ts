// export const fractalLoginWarning = {
//     NONE: "",
//     INVALID: "Invalid email or password",
// }

// export const fractalSignupWarning = {
//     NONE: "",
//     ACCOUNT_EXISTS: "Email already registered. Please log in instead.",
// }
//

export const WarningSignupInvalid =
    "Email already registered, please log in instead"

export const WarningLoginInvalid = "Invalid email or password"

export const ContainerAssignTimeout = 45000

// The Electron BrowserWindow API can be passed a hash parameter as data.
// We use this so that renderer threads can decide which view component to
// render as soon as a window appears.
export const WindowHashAuth = "AUTH"

export const WindowHashAuthError = "AUTH_ERROR"

export const WindowHashContainerError = "CONTAINER_ERROR"

export const WindowHashProtocolError = "PROTOCOL_ERROR"

export const AuthErrorTitle = "There was an error logging you in."

export const AuthErrorText =
    "Try again in a few minutes, or contact support@fractal.co for help."

export const ContainerErrorTitle =
    "There was an error connecting to the Fractal servers."

export const ContainerErrorText =
    "Try again in a few minutes, or contact support@fractal.co for help."

export const ProtocolErrorTitle = "The Fractal browser encountered an error."

export const ProtocolErrorText =
    "Try again in a few minutes, or contact support@fractal.co for help."

export const NavigationErrorTitle = "There was an error loading this window."

export const NavigationErrorText =
    "Restart the Fractal application, or email support@fractal.co for help. "

export const StateChannel = "MAIN_STATE_CHANNEL"

export const ErrorIPC = [
    "Before you call useMainState(),",
    "an ipcRenderer must be attached to the renderer window object to",
    "communicate with the main process.",
    "\n\nYou need to attach it in an Electron preload.js file using",
    "contextBridge.exposeInMainWorld. You must explicity attach the 'on' and",
    "'send' methods for them to be exposed.",
].join(" ")
