import { initFileSyncHandler } from "./downloads"
import { initTabDetachSuppressor } from "./tabs"
import { initChromeWelcomeRedirect } from "./navigation"
import { initNativeHostIpc } from "./ipc"
import { initCursorLockHandler } from "./cursor"
import { refreshExtension } from "./update"

const nativeHostPort = initNativeHostIpc()
// If this is a new mandelbox, refresh the extension to get the latest version.
refreshExtension(nativeHostPort)
initFileSyncHandler(nativeHostPort)
initCursorLockHandler(nativeHostPort)
initTabDetachSuppressor()
initChromeWelcomeRedirect()
