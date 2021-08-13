import { app, IpcMainEvent, BrowserWindow } from "electron"
import {
  takeUntil,
  withLatestFrom,
  startWith,
  mapTo,
  throttle,
} from "rxjs/operators"
import { merge, interval } from "rxjs"

import { destroyTray } from "@app/utils/tray"
import { uploadToS3, logBase } from "@app/utils/logging"
import { fromTrigger } from "@app/utils/flows"
import {
  WindowHashProtocol,
  WindowHashNetworkWarning,
} from "@app/utils/constants"
import { hideAppDock } from "@app/utils/dock"
import {
  createNetworkWarningWindow,
  createTypeformWindow,
  getElectronWindows,
} from "@app/utils/windows"
import { store, persist } from "@app/utils/persist"

const quit = () => {
  hideAppDock()
  app.quit()
}

fromTrigger("windowsAllClosed")
  .pipe(
    takeUntil(
      merge(fromTrigger("updateDownloaded"), fromTrigger("updateAvailable"))
    )
  )
  .subscribe((evt: IpcMainEvent) => {
    evt?.preventDefault()
  })

fromTrigger("windowInfo")
  .pipe(
    takeUntil(
      merge(fromTrigger("updateDownloaded"), fromTrigger("updateAvailable"))
    ),
    withLatestFrom(
      fromTrigger("mandelboxFlowFailure").pipe(mapTo(true), startWith(false))
    )
  )
  .subscribe(
    ([args]: [
      {
        numberWindowsRemaining: number
        crashed: boolean
        event: string
        hash: string
      },
      boolean
    ]) => {
      // If there are still windows open, ignore
      if (args.numberWindowsRemaining !== 0) return
      // If all windows are closed and the protocol wasn't the last open window, quit
      logBase("Application exited", {})
        .then(() => {
          if (store.get("data.exitSurveySubmitted") === undefined) {
            createTypeformWindow("https://form.typeform.com/to/Yfs4GkeN")
            persist("exitSurveySubmitted", true, "data")
          } else if (args.hash !== WindowHashProtocol) {
            quit()
          } else {
            // If the protocol was the last window to be closed, upload logs and quit the app
            destroyTray()
            uploadToS3()
              .then(() => {
                if (!args.crashed) quit()
              })
              .catch((err) => {
                console.error(err)
                if (!args.crashed) quit()
              })
          }
        })
        .catch((err) => console.log(err))
    }
  )

fromTrigger("networkUnstable")
  .pipe(throttle(() => interval(500))) // Throttle to 0.5s so we don't flood the main thread
  .subscribe((unstable: boolean) => {
    let warningWindowOpen = false
    getElectronWindows().forEach((win: BrowserWindow) => {
      if (win.webContents.getURL().includes(WindowHashNetworkWarning)) {
        warningWindowOpen = true
        if (!unstable) {
          win.close()
        }
      }
    })

    if (!warningWindowOpen && unstable) createNetworkWarningWindow()
  })
