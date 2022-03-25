import { initFileSyncHandler } from "./downloads"
import { initChromeWelcomeRedirect } from "./navigation"
import { initNativeHostIpc, initNativeHostDisconnectHandler } from "./ipc"
import { initCursorLockHandler } from "./cursor"
import { refreshExtension } from "./update"
import {
  initTabDetachSuppressor,
  initCreateNewTabHandler,
  initActivateTabHandler,
} from "./tabs"

const nativeHostPort = initNativeHostIpc()

// Disconnects the host native port on command
initNativeHostDisconnectHandler(nativeHostPort)

refreshExtension(nativeHostPort)
initFileSyncHandler(nativeHostPort)

// Enables relative mouse mode
initCursorLockHandler(nativeHostPort)

// Prevents tabs from being dragged out to new windows
initTabDetachSuppressor()

// Redirects the chrome://welcome page to our own Whist-branded page
initChromeWelcomeRedirect()

// Creates a new tab if the client asks for one
initCreateNewTabHandler(nativeHostPort)

// Switches focus to a current tab if the client asks for it
initActivateTabHandler(nativeHostPort)
