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
  initializeWhistFreezeHandler,
  initializeWhistThawHandler,
  initializeRequestMandelbox,
} from "./controls/whist"
import { initializePageRefreshHandler } from "./controls/refresh"

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
  })

  initializeLoadingScreen()
  initializeWhistTagHandler()
  initializeWhistFreezeHandler()
  initializeWhistThawHandler()
  initializeRequestMandelbox()
}
