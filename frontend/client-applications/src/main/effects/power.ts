/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file app.ts
 * @brief This file contains effects that deal with the app lifecycle (e.g. relaunching, quitting, etc.)
 */

import { app, session } from "electron"
import { merge } from "rxjs"
import { withLatestFrom, filter, take, takeUntil } from "rxjs/operators"
import { ChildProcess } from "child_process"
import Sentry from "@sentry/electron"

import { fromTrigger } from "@app/main/utils/flows"
import { relaunch } from "@app/main/utils/app"
import { WhistTrigger } from "@app/constants/triggers"
import { logToAmplitude } from "@app/main/utils/logging"
import { persistClear } from "@app/main/utils/persist"
import { destroyProtocol } from "@app/main/utils/protocol"
import { emitOnSignal } from "@app/main/utils/observables"

// Handles the application quit logic
// When we detect that all windows have been closed, we put the application to sleep
// i.e. keep it active in the user's dock but don't show any windows
const shouldSleep = merge(
  // If all Electron windows have closed and the protocol isn't connected
  fromTrigger(WhistTrigger.electronWindowsAllClosed).pipe(
    withLatestFrom(fromTrigger(WhistTrigger.protocolConnection)),
    filter(([, connected]: [any, boolean]) => !connected)
  ),
  // If the protocol was closed gracefully and all Electron windows are closed
  fromTrigger(WhistTrigger.protocolClosed).pipe(
    filter((args: { crashed: boolean }) => !args.crashed)
  ),
  fromTrigger(WhistTrigger.powerResume)
).pipe(
  takeUntil(
    merge(
      fromTrigger(WhistTrigger.relaunchAction),
      fromTrigger(WhistTrigger.clearCacheAction),
      fromTrigger(WhistTrigger.updateDownloaded).pipe(
        takeUntil(fromTrigger(WhistTrigger.mandelboxFlowSuccess))
      ),
      fromTrigger(WhistTrigger.userRequestedQuit),
      fromTrigger(WhistTrigger.stripeAuthRefresh).pipe(
        withLatestFrom(fromTrigger(WhistTrigger.protocolConnection)),
        filter(([, connected]) => !connected)
      )
    )
  ),
  take(1) // When we relaunch we reset the application; this ensures we don't relaunch multiple times
)

/* eslint-disable @typescript-eslint/no-misused-promises */
shouldSleep
  .pipe(withLatestFrom(fromTrigger(WhistTrigger.protocol)))
  .subscribe(async ([, p]: [any, ChildProcess]) => {
    await logToAmplitude("Whist sleeping")

    destroyProtocol(p)
    relaunch({ sleep: true })
  })

// On signout, clear the cache (so the user can log in again) and restart
// the app
emitOnSignal(
  fromTrigger(WhistTrigger.protocol),
  fromTrigger(WhistTrigger.clearCacheAction)
).subscribe((p: ChildProcess) => {
  persistClear()
  // Clear the Auth0 cache. In window.ts, we tell Auth0 to store session info in
  // a partition called "auth0", so we clear the "auth0" partition here
  session.defaultSession
    .clearStorageData()
    .catch((err) => Sentry.captureException(err))

  // Restart the app
  destroyProtocol(p)
  relaunch({ sleep: false })
})

// If the user requests a relaunch
emitOnSignal(
  fromTrigger(WhistTrigger.protocol),
  fromTrigger(WhistTrigger.relaunchAction)
).subscribe((p: ChildProcess) => {
  destroyProtocol(p)
  relaunch({ sleep: false })
})

fromTrigger(WhistTrigger.stripeAuthRefresh)
  .pipe(
    withLatestFrom(fromTrigger(WhistTrigger.protocolConnection)),
    filter(([, connected]) => !connected)
  )
  .subscribe(() => {
    relaunch({ sleep: false })
  })

/* eslint-disable @typescript-eslint/no-misused-promises */
fromTrigger(WhistTrigger.userRequestedQuit).subscribe(async () => {
  await logToAmplitude("Whist force quit")
  app.exit()
})
