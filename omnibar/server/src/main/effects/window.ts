import { appReady } from "@app/main/triggers/electron"
import { createBrowserView, attachBrowserView } from "@app/utils/view"
import { createBrowserWindow } from "@app/utils/window"

appReady.subscribe(() => {
  const window = createBrowserWindow()
  const view = createBrowserView("https://google.com")
  attachBrowserView(view, window)
  window.show()
})
