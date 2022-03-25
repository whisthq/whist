import { initFileSyncHandler } from "./downloads"
import { initChromeWelcomeRedirect } from "./navigation"
import { initNativeHostIpc } from "./ipc"
import { initCursorLockHandler } from "./cursor"
import { refreshExtension } from "./update"
import {
  initTabDetachSuppressor,
  initTabCreationHandler,
  initTabSwitchingHandler,
} from "./tabs"

const nativeHostPort = initNativeHostIpc()

// If this is a new mandelbox, refresh the extension to get the latest version.
refreshExtension(nativeHostPort)
initFileSyncHandler(nativeHostPort)
initCursorLockHandler(nativeHostPort)
initTabDetachSuppressor()
initTabCreationHandler()
initTabSwitchingHandler()
initChromeWelcomeRedirect()
