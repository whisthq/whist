/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file app.ts
 * @brief This file contains effects that deal with electron-updater
 */

import { autoUpdater } from "electron-updater"
import { timer } from "rxjs"
import { takeUntil } from "rxjs/operators"
import Sentry from "@sentry/electron"

import { appEnvironment, WhistEnvironments } from "../../../config/configs"
import { fromTrigger } from "@app/main/utils/flows"
import { WhistTrigger } from "@app/constants/triggers"
import { createUpdateWindow } from "@app/main/utils/renderer"
import { withAppActivated } from "@app/main/utils/observables"
import { CHECK_UPDATE_INTERVAL_IN_MS } from "@app/constants/app"

// If an update is available, show the update window and download the update
fromTrigger(WhistTrigger.updateAvailable).subscribe(() => {
  autoUpdater.downloadUpdate().catch((err) => Sentry.captureException(err))
})

withAppActivated(timer(0, CHECK_UPDATE_INTERVAL_IN_MS)).subscribe(() => {
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
    .catch((err) => Sentry.captureException(err))
})

withAppActivated(
  fromTrigger(WhistTrigger.updateAvailable).pipe(
    takeUntil(fromTrigger(WhistTrigger.mandelboxFlowSuccess))
  )
).subscribe(() => {
  createUpdateWindow()
})

fromTrigger(WhistTrigger.updateDownloaded)
  .pipe(takeUntil(fromTrigger(WhistTrigger.mandelboxFlowSuccess)))
  .subscribe(() => {
    autoUpdater.quitAndInstall()
  })
