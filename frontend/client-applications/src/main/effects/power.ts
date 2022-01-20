import { BrowserWindow } from "electron"
import { merge } from "rxjs"
import { withLatestFrom, filter, takeUntil } from "rxjs/operators"

import { fromTrigger } from "@app/main/utils/flows"
import { destroyProtocol } from "@app/main/utils/protocol"
import { relaunch } from "@app/main/utils/app"
import { emitOnSignal, waitForSignal } from "@app/main/utils/observables"
import { WhistTrigger } from "@app/constants/triggers"
import { logBase } from "@app/main/utils/logging"

// Handles the application quit logic
// When we detect that all windows have been closed, we put the application to sleep
// i.e. keep it active in the user's dock but don't show any windows
merge(
  // If all Electron windows have closed and the protocol isn't connected
  fromTrigger(WhistTrigger.electronWindowsAllClosed).pipe(
    withLatestFrom(fromTrigger(WhistTrigger.protocolConnected)),
    filter(([, connected]) => !connected)
  ),
  // If the protocol was closed gracefully and all Electron windows are closed
  fromTrigger(WhistTrigger.protocolClosed).pipe(
    filter((args: { crashed: boolean }) => !args.crashed),
    filter(() => BrowserWindow.getAllWindows()?.length === 0)
  )
)
  .pipe(
    takeUntil(
      merge(
        fromTrigger(WhistTrigger.relaunchAction),
        fromTrigger(WhistTrigger.clearCacheAction),
        fromTrigger(WhistTrigger.updateDownloaded),
        fromTrigger(WhistTrigger.userRequestedQuit)
      )
    )
  )
  .subscribe(() => {
    logBase("Application sleeping", {})
    relaunch({ sleep: true })
  })

// If you put your computer to sleep, kill the protocol so we don't keep your mandelbox
// running unnecessarily
emitOnSignal(
  fromTrigger(WhistTrigger.protocol),
  fromTrigger(WhistTrigger.powerSuspend)
).subscribe((protocol) => destroyProtocol(protocol))

// If your computer wakes up from sleep, re-launch Whist on standby
// i.e. don't show any windows but stay active in the dock
fromTrigger(WhistTrigger.powerResume).subscribe(() => {
  relaunch({ sleep: true })
})
