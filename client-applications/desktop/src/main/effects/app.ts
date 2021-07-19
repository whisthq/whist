/**
 * Copyright Fractal Computers, Inc. 2021
 * @file app.ts
 * @brief This file contains subscriptions to Electron app event emitters observables.
 */

import { app, IpcMainEvent, session } from "electron"
import { autoUpdater } from "electron-updater"
import { take, takeUntil } from "rxjs/operators"
import { merge } from "rxjs"

import { AWSRegion } from "@app/@types/aws"
import {
  createAuthWindow,
  createUpdateWindow,
  createSignoutWindow,
  createProtocolWindow,
  closeAllWindows,
  relaunch,
  createPaymentWindow,
  createTypeformWindow,
} from "@app/utils/windows"
import { createTray, destroyTray } from "@app/utils/tray"
import { uploadToS3 } from "@app/utils/logging"
import { appEnvironment, FractalEnvironments } from "../../../config/configs"
import { fromTrigger } from "@app/utils/flows"
import { emitAuthCache, persistClear } from "@app/utils/persist"

// Apply autoupdate config
fromTrigger("appReady")
  .pipe(take(1), takeUntil(fromTrigger("updateChecking")))
  .subscribe(() => {
    // We want to manually control when we download the update via autoUpdater.quitAndInstall(),
    // so we need to set autoDownload = false
    autoUpdater.autoDownload = false
    // In dev and staging, the file containing the version is called {channel}-mac.yml, so we need to set the
    // channel down below. In prod, the file is called latest-mac.yml, which channel defaults to, so
    // we don't need to set it.
    switch (appEnvironment) {
      case FractalEnvironments.STAGING:
        autoUpdater.channel = "staging-rc"
        break
      case FractalEnvironments.DEVELOPMENT:
        autoUpdater.channel = "dev-rc"
        break
      default:
        break
    }

    // This is what looks for a latest.yml file in the S3 bucket in electron-builder.config.js,
    // and fires an update if the current version is less than the version in latest.yml
    autoUpdater.checkForUpdatesAndNotify().catch((err) => console.error(err))
  })

// Check Electron store for persisted data
fromTrigger("appReady").subscribe(() => emitAuthCache())

// appReady only fires once, at the launch of the application.
// We use takeUntil to make sure that the auth window only fires when
// we have all of [userEmail, userAccessToken, userConfigToken]. If we
// don't have all three, we clear them all and force the user to log in again.
fromTrigger("notPersisted").subscribe(() => {
  createAuthWindow()
})

// If we have have successfully authorized, close the existing windows.
// It's important to put this effect after the application closing effect.
// If not, the filters on the application closing observable don't run.
// This causes the app to close on every loginSuccess, before the protocol
// can launch.
fromTrigger("authFlowSuccess").subscribe((x: { userEmail: string }) => {
  createProtocolWindow().catch((err) => console.error(err))
  createTray(x.userEmail)
})

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
  .subscribe((args: { numberWindowsRemaining: number; crashed: boolean }) => {
    if (args.numberWindowsRemaining === 0) {
      destroyTray()
      uploadToS3()
        .then(() => {
          if (!args.crashed) app.quit()
        })
        .catch((err) => {
          console.error(err)
          if (!args.crashed) app.quit()
        })
    }
  })

fromTrigger("trayQuitAction").subscribe(() => {
  closeAllWindows()
})

// If the update is downloaded, quit the app and install the update
fromTrigger("updateDownloaded")
  .pipe(take(1))
  .subscribe(() => {
    autoUpdater.quitAndInstall()
  })

// If an update is available, show the update window and download the update
fromTrigger("updateAvailable")
  .pipe(take(1))
  .subscribe(() => {
    createUpdateWindow()
    autoUpdater.downloadUpdate().catch((err) => console.error(err))
  })

// On signout or relaunch, clear the cache (so the user can log in again) and restart
// the app
fromTrigger("clearCacheAction").subscribe(() => {
  persistClear()
  // Clear the Auth0 cache. In window.ts, we tell Auth0 to store session info in
  // a partition called "auth0", so we clear the "auth0" partition here
  session
    .fromPartition("auth0")
    .clearStorageData()
    .catch((err) => console.error(err))
  // Restart the app
  relaunch()
})

// If an admin selects a region, relaunch the app with the selected region passed
// into argv so it can be read by flows/index.ts
fromTrigger("trayRegionAction").subscribe((region: AWSRegion) => {
  relaunch({ args: process.argv.slice(1).concat([region]) })
})

fromTrigger("relaunchAction").subscribe(() => {
  relaunch()
})

fromTrigger("showSignoutWindow").subscribe(() => {
  createSignoutWindow()
})

fromTrigger("trayFeedbackAction").subscribe(() => {
  createTypeformWindow()
})

// eslint-disable-next-line @typescript-eslint/no-misused-promises
fromTrigger("showPaymentWindow").subscribe(async () => {
  await createPaymentWindow()
})
