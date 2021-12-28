import { app } from "electron"

import {
  createOmnibar,
  hideOmnibar,
  showOmnibar,
  omniBar,
} from "@app/utils/navigation/omnibar"
import { fromTrigger } from "@app/utils/rxjs/triggers"
import { FractalTrigger } from "@app/utils/constants/triggers"
import {
  getBrowserWindow,
  showBrowserWindow,
} from "@app/utils/navigation/browser"
import { navigate } from "@app/utils/navigation/url"
import { displayView, getView, destroyView } from "@app/utils/navigation/view"
import { getSearchableUrl } from "@app/utils/navigation/url"
import { createTray } from "@app/utils/tray"

// Create the omnibar on app start
fromTrigger(FractalTrigger.appReady).subscribe(() => {
  createOmnibar()
  createTray()
})

// This handles the "open in new tab" command
fromTrigger(FractalTrigger.createView).subscribe(
  (payload: { text: string }) => {
    // hideOmnibar();
    navigate(payload.text)
    showBrowserWindow()
  }
)

// This handles the "open in current tab" command
fromTrigger(FractalTrigger.changeViewUrl).subscribe(
  (payload: { id: number; text: string }) => {
    const win = getBrowserWindow()
    const view = getView(payload.id, win)
    view?.webContents.loadURL(getSearchableUrl(payload.text))
    view !== undefined && displayView(view, win)
    showBrowserWindow()
  }
)

// This handles the "display current view" on hover over tab
// If the user clicks on a view in the omnibar, open it
fromTrigger(FractalTrigger.displayView).subscribe(
  (payload: { id: number; hideOmnibar: boolean }) => {
    if (payload.hideOmnibar) hideOmnibar()

    const win = getBrowserWindow()
    const view = getView(payload?.id, win)

    if (view !== undefined) displayView(view, win) // TODO: Handle undefined case
    showBrowserWindow(false)
  }
)

// This handles the close view command
fromTrigger(FractalTrigger.closeView).subscribe((payload: { id: number }) => {
  const win = getBrowserWindow()

  const view = getView(payload?.id, win)
  if (view !== undefined) destroyView(view, win)
})

// If only the omnibar is left, show it
fromTrigger(FractalTrigger.browserWindowClosed).subscribe(() => {
  if (getBrowserWindow() === undefined && omniBar !== undefined) {
    showOmnibar()
  }
})
