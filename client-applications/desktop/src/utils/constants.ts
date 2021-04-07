// TODO: This is getting a bit unwieldy. Let's create a way to group related errors, e.g.
// a "container error" object.

export const HostServicePort = 4678

export const WarningSignupInvalid =
    "Email already registered, please log in instead"

export const WarningLoginInvalid = "Invalid email or password"

export const ContainerAssignTimeout = 120000

// The Electron BrowserWindow API can be passed a hash parameter as data.
// We use this so that renderer threads can decide which view component to
// render as soon as a window appears.
export const WindowHashAuth = "AUTH"

export const WindowHashUpdate = "UPDATE"

export const WindowHashAuthError = "AUTH_ERROR"

export const WindowHashCreateContainerErrorNoAccess =
    "CREATE_CONTAINER_ERROR_NO_ACCESS"

export const WindowHashCreateContainerErrorUnauthorized =
    "CREATE_CONTAINER_ERROR_UNAUTHORIZED"

export const WindowHashCreateContainerErrorInternal =
    "CREATE_CONTAINER_ERROR_INTERNAL"

export const WindowHashAssignContainerError = "ASSIGN_CONTAINER_ERROR"

export const WindowHashProtocolError = "PROTOCOL_ERROR"

export const AuthErrorTitle = "There was an error logging you in."

export const AuthErrorText =
    "Try again in a few minutes, or contact support@fractal.co for help."

export const ContainerCreateErrorTitleInternal =
    "There was an error connecting to the Fractal servers."

export const ContainerCreateErrorTextInternal =
    "Try again in a few minutes, or contact support@fractal.co for help."

export const ContainerCreateErrorTitleNoAccess =
    "Your account does not have access to Fractal."

export const ContainerCreateErrorTextNoAccess =
    "Access to Fractal is currently invite-only. Please contact support@fractal.co for help."

export const ContainerCreateErrorTitleUnauthorized =
    "There was an error authenticating you with the Fractal servers."

export const ContainerCreateErrorTextUnauthorized =
    "Please try logging in again."

export const ContainerAssignErrorTitle =
    "Fractal servers experienced an unexpected error."

export const ContainerAssignErrorText =
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
