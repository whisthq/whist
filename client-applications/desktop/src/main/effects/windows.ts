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
    )
  )
  .subscribe(
    (args: {
      numberWindowsRemaining: number
      crashed: boolean
      event: string
      hash: string
    }) => {
      // If there are still windows open, ignore
      if (args.numberWindowsRemaining !== 0) return
      // If all windows are closed and the protocol wasn't the last open window, quit
      if (
        args.hash !== WindowHashProtocol ||
        (args.hash === WindowHashProtocol && !args.crashed)
      ) {
        logBase("Application exited", {}).then(() => {
          quit()
        })
      }
    }
  )

fromTrigger("windowInfo")
  .pipe(
    takeUntil(
      merge(fromTrigger("updateDownloaded"), fromTrigger("updateAvailable"))
    ),
    withLatestFrom(fromTrigger("mandelboxFlowSuccess"))
  )
  .subscribe(
    ([args, info]: [
      {
        numberWindowsRemaining: number
        crashed: boolean
        event: string
        hash: string
      },
      {
        mandelboxIP: string
        mandelboxSecret: string
        mandelboxPorts: {
          port_32262: number
          port_32263: number
          port_32273: number
        }
      }
    ]) => {
      if (
        args.hash === WindowHashProtocol &&
        args.crashed &&
        args.event === "close"
      ) {
        if (protocolLaunchRetries < MAX_RETRIES) {
          protocolLaunchRetries = protocolLaunchRetries + 1
          createProtocolWindow().then(() => {
            protocolStreamInfo(info)
            const win = createRelaunchWarningWindow()
            setTimeout(() => {
              win?.close()
            }, 6000)
          })
        } else {
          createErrorWindow(PROTOCOL_ERROR)
        }
      }
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
