import { initFileSyncHandler } from "./downloads"
import { initChromeWelcomeRedirect } from "./navigation"
import {
  initTabDetachSuppressor,
  initTabCreationHandler,
  initTabSwitchingHandler,
} from "./tabs"

initFileSyncHandler()
initTabDetachSuppressor()
initTabCreationHandler()
initTabSwitchingHandler()
initChromeWelcomeRedirect()
