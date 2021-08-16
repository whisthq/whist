import { app, IpcMainEvent, BrowserWindow } from "electron"
import {
  takeUntil,
  withLatestFrom,
  startWith,
  mapTo,
  throttle,
  filter,
} from "rxjs/operators"
import { merge, interval } from "rxjs"

import { destroyTray } from "@app/utils/tray"
import { logBase, uploadToS3 } from "@app/utils/logging"
import { fromTrigger } from "@app/utils/flows"
import {
  WindowHashProtocol,
  WindowHashNetworkWarning,
} from "@app/utils/constants"
import { hideAppDock } from "@app/utils/dock"
import {
  createErrorWindow,
  createNetworkWarningWindow,
  createRelaunchWarningWindow,
  createProtocolWindow,
  createTypeformWindow,
  getElectronWindows,
} from "@app/utils/windows"
import { store, persist } from "@app/utils/persist"
import { protocolStreamInfo } from "@app/utils/protocol"
import { PROTOCOL_ERROR } from "@app/utils/error"

let protocolLaunchRetries = 0
const MAX_RETRIES = 3

const quit = () => {
  logBase("Application exited", {})
    .then(async () => await uploadToS3())
    .then(() => {
      destroyTray()
      hideAppDock()
      app.quit()
    })
    .catch((err) => {
      logBase("Error while quitting", { error: err }).catch((err) =>
        console.error(err)
      )
      destroyTray()
      hideAppDock()
      app.quit()
    })
}

const allWindowsClosed = fromTrigger("windowInfo").pipe(
  takeUntil(
    merge(fromTrigger("updateDownloaded"), fromTrigger("updateAvailable"))
  ),
  filter(
    (args: {
      crashed: boolean
      numberWindowsRemaining: number
      hash: string
      event: string
    }) => args.numberWindowsRemaining === 0
  )
)

fromTrigger("windowsAllClosed")
  .pipe(
    takeUntil(
      merge(fromTrigger("updateDownloaded"), fromTrigger("updateAvailable"))
    )
  )
  .subscribe((evt: IpcMainEvent) => {
    evt?.preventDefault()
  })

allWindowsClosed
  .pipe(
    withLatestFrom(fromTrigger("mandelboxFlowSuccess")),
    withLatestFrom(
      fromTrigger("mandelboxFlowFailure").pipe(mapTo(true), startWith(false))
    )
  )
  .subscribe(
    ([[args, info], mandelboxFailure]: [
      [
        {
          crashed: boolean
          numberWindowsRemaining: number
          hash: string
          event: string
        },
        any
      ],
      boolean
    ]) => {
      // If they didn't crash out and didn't fill out the exit survey, show it to them
      if (
        store.get("data.exitSurveySubmitted") === undefined &&
        !mandelboxFailure &&
        !args.crashed
      ) {
        createTypeformWindow("https://form.typeform.com/to/Yfs4GkeN")
        persist("exitSurveySubmitted", true, "data")
        // If all windows were successfully closed, quit
      } else if (
        args.hash !== WindowHashProtocol ||
        (args.hash === WindowHashProtocol && !args.crashed)
      ) {
        quit()
        // If the protocol crashed out, try to reconnect
      } else if (
        args.hash === WindowHashProtocol &&
        args.crashed &&
        args.event === "close"
      ) {
        if (protocolLaunchRetries < MAX_RETRIES) {
          protocolLaunchRetries = protocolLaunchRetries + 1
          createProtocolWindow()
            .then(() => {
              protocolStreamInfo(info)
              const win = createRelaunchWarningWindow()
              setTimeout(() => {
                win?.close()
              }, 6000)
            })
            .catch((err) => console.error(err))
        }
        // If we've already tried several times to reconnect, just show the protocol error window
      } else {
        createErrorWindow(PROTOCOL_ERROR)
      }
    }
  )

fromTrigger("networkUnstable")
  .pipe(throttle(() => interval(1000))) // Throttle to 0.5s so we don't flood the main thread
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
