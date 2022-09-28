import { initializeSessionStorage } from "./controls/session"
import {
  initializeGetTabId,
  initializeFaviconUpdater,
  initializeTitleUpdater,
} from "./controls/tab"
import {
  initializeURLUpdater,
  initializePopStateHandler,
} from "./controls/history"
import {
  initializeWhistTagHandler,
  initializeWhistFreezeAllHandler,
  initializeWhistSpotlightHandler,
  initializeRequestMandelbox,
} from "./controls/whist"
import { initializePageRefreshHandler } from "./controls/refresh"
import { initializeMouseEnterHandler } from "./controls/mouse"

import {
  createStartNotification,
  createDisconnectNotification,
} from "./ui/notifications"
import { initializeLoadingScreen } from "./ui/loading"

window.onload = () => {
  initializeSessionStorage()
  createStartNotification()
  createDisconnectNotification()

  initializeGetTabId().then(() => {
    initializeURLUpdater()
    initializePopStateHandler()
    initializeTitleUpdater()
    initializeFaviconUpdater()
    initializePageRefreshHandler()
    initializeMouseEnterHandler()
  })

  initializeLoadingScreen()
  initializeWhistTagHandler()
  initializeWhistFreezeAllHandler()
  initializeWhistSpotlightHandler()
  initializeRequestMandelbox()
}
