import { app, IpcMainEvent } from "electron"
import { takeUntil, withLatestFrom, startWith, mapTo } from "rxjs/operators"
import { merge } from "rxjs"

import { destroyTray } from "@app/utils/tray"
import { uploadToS3, logBase } from "@app/utils/logging"
import { fromTrigger } from "@app/utils/flows"
import { WindowHashProtocol } from "@app/utils/constants"
import { hideAppDock } from "@app/utils/dock"

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
    ([args, mandelboxFailed]: [
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

      if (args.hash !== WindowHashProtocol) {
        quit()
        return
      }

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
  )
