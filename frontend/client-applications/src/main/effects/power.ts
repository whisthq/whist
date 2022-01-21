/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file app.ts
 * @brief This file contains effects that deal with the app lifecycle (e.g. relaunching, quitting, etc.)
 */

import { BrowserWindow, session } from "electron"
import { merge } from "rxjs"
import { withLatestFrom, filter, takeUntil } from "rxjs/operators"
import Sentry from "@sentry/electron"

import { fromTrigger } from "@app/main/utils/flows"
import { relaunch } from "@app/main/utils/app"
import { WhistTrigger } from "@app/constants/triggers"
import { logBase } from "@app/main/utils/logging"
import { persistClear } from "@app/main/utils/persist"

// Handles the application quit logic
// When we detect that all windows have been closed, we put the application to sleep
// i.e. keep it active in the user's dock but don't show any windows
merge(
  // If all Electron windows have closed and the protocol isn't connected
  fromTrigger(WhistTrigger.electronWindowsAllClosed).pipe(
    withLatestFrom(fromTrigger(WhistTrigger.protocolConnection)),
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
        fromTrigger(WhistTrigger.updateDownloaded).pipe(
          takeUntil(fromTrigger(WhistTrigger.mandelboxFlowSuccess))
        ),
        fromTrigger(WhistTrigger.userRequestedQuit)
      )
    )
  )
  .subscribe(() => {
    logBase("Application sleeping", {})
    relaunch({ sleep: true })
  })

// If your computer wakes up from sleep, re-launch Whist on standby
// i.e. don't show any windows but stay active in the dock
fromTrigger(WhistTrigger.powerResume).subscribe(() => {
  relaunch({ sleep: true })
})

// On signout or relaunch, clear the cache (so the user can log in again) and restart
// the app
fromTrigger(WhistTrigger.clearCacheAction).subscribe(() => {
  persistClear()
  // Clear the Auth0 cache. In window.ts, we tell Auth0 to store session info in
  // a partition called "auth0", so we clear the "auth0" partition here
  session.defaultSession
    .clearStorageData()
    .catch((err) => Sentry.captureException(err))
  // Restart the app
  relaunch()
})

// If the user requests a relaunch
merge(fromTrigger(WhistTrigger.relaunchAction)).subscribe(() => {
  relaunch()
})
