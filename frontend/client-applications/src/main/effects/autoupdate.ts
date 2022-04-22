/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file app.ts
 * @brief This file contains effects that deal with electron-updater
 */

import { BrowserWindow } from "electron"
import { autoUpdater } from "electron-updater"
import { timer, merge } from "rxjs"
import { takeUntil, withLatestFrom, take } from "rxjs/operators"
import { ChildProcess } from "child_process"
import * as Sentry from "@sentry/electron"

import { CHECK_UPDATE_INTERVAL_IN_MS, sessionID } from "@app/constants/app"

import { appEnvironment, WhistEnvironments } from "../../../config/configs"
import { fromTrigger } from "@app/main/utils/flows"
import { WhistTrigger } from "@app/constants/triggers"
import { createUpdateWindow } from "@app/main/utils/renderer"
import { withAppActivated } from "@app/main/utils/observables"
import { destroyProtocol } from "@app/main/utils/protocol"

// If an update is available, show the update window and download the update
fromTrigger(WhistTrigger.updateAvailable).subscribe(() => {
  autoUpdater
    .downloadUpdate()
    .catch((err: Error) =>
      Sentry.captureMessage(`Error downloading update: ${err.message}`)
    )
})

timer(0, CHECK_UPDATE_INTERVAL_IN_MS)
  .pipe(takeUntil(fromTrigger(WhistTrigger.updateAvailable)))
  .subscribe(() => {
    // We want to manually control when we download the update via autoUpdater.quitAndInstall(),
    // so we need to set autoDownload = false
    autoUpdater.autoDownload = false
    // In dev and staging, the file containing the version is called {channel}-mac.yml, so we need to set the
    // channel down below. In prod, the file is called latest-mac.yml, which channel defaults to, so
    // we don't need to set it.
    switch (appEnvironment) {
      case WhistEnvironments.PRODUCTION:
        break
      case WhistEnvironments.STAGING:
        autoUpdater.channel = "staging-rc"
        break
      case WhistEnvironments.DEVELOPMENT:
        autoUpdater.channel = "dev-rc"
        break
      default:
        autoUpdater.channel = "dev-rc"
        break
    }

    // This is what looks for a latest.yml file in the S3 bucket in electron-builder.config.js,
    // and fires an update if the current version is less than the version in latest.yml
    autoUpdater
      .checkForUpdatesAndNotify()
      .catch((err: Error) =>
        Sentry.captureMessage(`Error checking for update: ${err.message}`)
      )
  })

withAppActivated(
  merge(
    fromTrigger(WhistTrigger.updateAvailable),
    fromTrigger(WhistTrigger.updateDownloaded)
  ).pipe(takeUntil(fromTrigger(WhistTrigger.mandelboxFlowSuccess)), take(1))
).subscribe(() => {
  const openWindows = BrowserWindow.getAllWindows()

  createUpdateWindow()

  openWindows.forEach((win) => {
    win?.destroy()
  })
})

withAppActivated(
  fromTrigger(WhistTrigger.updateDownloaded).pipe(
    withLatestFrom(fromTrigger(WhistTrigger.protocol)),
    takeUntil(fromTrigger(WhistTrigger.mandelboxFlowSuccess))
  )
).subscribe(([, p]: [any, ChildProcess]) => {
  const msElapsed = Date.now() - sessionID
  const delay = Math.max(0, 4000 - msElapsed)

  setTimeout(() => {
    destroyProtocol(p)
    autoUpdater.quitAndInstall()
  }, delay)
})
