import { app, IpcMainEvent, Notification } from "electron"
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
import { logBase } from "@app/utils/logging"
import { fromTrigger } from "@app/utils/flows"
import { WindowHashProtocol } from "@app/utils/constants"
import { hideAppDock } from "@app/utils/dock"
import {
  createErrorWindow,
  createProtocolWindow,
  createExitTypeform,
} from "@app/utils/windows"
import { persistGet } from "@app/utils/persist"
import { PROTOCOL_ERROR } from "@app/utils/error"
import { internetWarning, rebootWarning } from "@app/utils/notification"
import { protocolStreamInfo } from "@app/utils/protocol"

// Keeps track of how many times we've tried to relaunch the protocol
const MAX_RETRIES = 3
let protocolLaunchRetries = 0
// Notifications
let internetNotification: Notification | undefined
let rebootNotification: Notification | undefined
// Keeps track of how often we show warnings
let warningWindowOpen = false
let warningLastShown = 0
// Keeps track of if we've already asked them to fill out the exit survey
let exitSurveyShown = false

fromTrigger("appReady").subscribe(() => {
  internetNotification = internetWarning()
  rebootNotification = rebootWarning()
})

const quit = () => {
  logBase("Application exited", {})
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
  filter(
    (args: {
      crashed: boolean
      numberWindowsRemaining: number
      hash: string
      event: string
    }) => args.numberWindowsRemaining === 0
  )
)

// fromTrigger("windowsAllClosed")
//   .pipe(
//     takeUntil(
//       fromTrigger("installUpdate")
//     )
//   )
//   .subscribe((evt: IpcMainEvent) => {
//     evt?.preventDefault()
//   })

// allWindowsClosed
//   .pipe(
//     withLatestFrom(fromTrigger("mandelboxFlowSuccess").pipe(startWith({}))),
//     withLatestFrom(
//       fromTrigger("mandelboxFlowFailure").pipe(mapTo(true), startWith(false))
//     )
//   )
//   .subscribe(
//     ([[args, info], mandelboxFailure]: [
//       [
//         {
//           crashed: boolean
//           numberWindowsRemaining: number
//           hash: string
//           event: string
//         },
//         any
//       ],
//       boolean
//     ]) => {
//       // If they didn't crash out and didn't fill out the exit survey, show it to them
//       if (
//         persistGet("exitTypeformSubmitted", "data") === undefined &&
//         !mandelboxFailure &&
//         !args.crashed &&
//         !exitSurveyShown &&
//         args.hash === WindowHashProtocol
//       ) {
//         createExitTypeform()
//         exitSurveyShown = true
//         // If all windows were successfully closed, quit
//       } else if (
//         args.hash !== WindowHashProtocol ||
//         (args.hash === WindowHashProtocol && !args.crashed)
//       ) {
//         quit()
//         // If the protocol crashed out, try to reconnect
//       } else if (
//         args.hash === WindowHashProtocol &&
//         args.crashed &&
//         args.event === "close" &&
//         protocolLaunchRetries < MAX_RETRIES
//       ) {
//         protocolLaunchRetries = protocolLaunchRetries + 1
//         createProtocolWindow()
//           .then(() => {
//             protocolStreamInfo(info)
//             rebootNotification?.show()
//             setTimeout(() => {
//               rebootNotification?.close()
//             }, 6000)
//           })
//           .catch((err) => console.error(err))
//         // If we've already tried several times to reconnect, just show the protocol error window
//       } else {
//         createErrorWindow(PROTOCOL_ERROR)
//       }
//     }
//   )

fromTrigger("networkUnstable")
  .pipe(throttle(() => interval(1000))) // Throttle to 1s so we don't flood the main thread
  .subscribe((unstable: boolean) => {
    // Don't show the warning more than once within ten seconds
    if (
      !warningWindowOpen &&
      unstable &&
      Date.now() / 1000 - warningLastShown > 10
    ) {
      warningWindowOpen = true
      internetNotification?.show()
      warningLastShown = Date.now() / 1000
    }
    if (!unstable && warningWindowOpen) {
      internetNotification?.close()
      warningWindowOpen = false
    }
  })
