import { initFileSyncHandler } from "./downloads"
import { initTabDetachSuppressor } from "./tabs"
import { initChromeWelcomeRedirect } from "./navigation"

initFileSyncHandler()
initTabDetachSuppressor()
initChromeWelcomeRedirect()
