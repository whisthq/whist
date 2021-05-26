/**
 * Copyright Fractal Computers, Inc. 2021
 * @file app.ts
 * @brief This file contains subscriptions to Electron app event emitters observables.
 */

import { app, IpcMainEvent, session } from "electron"
import { autoUpdater } from "electron-updater"
import { fromEvent, merge, zip } from "rxjs"
import { mapTo, take, concatMap, pluck } from "rxjs/operators"
import path from "path"
import { ChildProcess } from "child_process"

import {
  closeWindows,
  createAuthWindow,
  createUpdateWindow,
  showAppDock,
  hideAppDock,
} from "@app/utils/windows"
import { createTray, destroyTray } from "@app/utils/tray"
import { uploadToS3 } from "@app/utils/logging"
import { appEnvironment, FractalEnvironments } from "../../../config/configs"
import config from "@app/config/environment"
import { fromTrigger } from "@app/utils/flows"
import { emitCache, persistClear } from "@app/utils/persist"

// Set custom app data folder based on environment
fromTrigger("appReady").subscribe(() => {
  const { deployEnv } = config
  const appPath = app.getPath("userData")
  const newPath = path.join(appPath, deployEnv)
  app.setPath("userData", newPath)
})

// Apply autoupdate config
fromTrigger("appReady")
  .pipe(take(1))
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
fromTrigger("appReady").subscribe(() => emitCache())

// appReady only fires once, at the launch of the application.
// We use takeUntil to make sure that the auth window only fires when
// we have all of [userEmail, userAccessToken, userConfigToken]. If we
// don't have all three, we clear them all and force the user to log in again.
fromTrigger("notPersisted").subscribe(() => {
  createAuthWindow()
})

// By default, the window-all-closed Electron event will cause the application
// to close. We don't want this behavior for certain observables. For example,
// when the protocol launches, we close all the windows, but we don't want the app
// to quit.
merge(
  fromTrigger("protocolLaunchFlowSuccess"),
  fromTrigger("authFlowSuccess"),
  fromTrigger("authFlowFailure"),
  fromTrigger("updateAvailable"),
  fromTrigger("protocolLaunchFlowFailure"),
  fromTrigger("containerFlowFailure")
)
  .pipe(concatMap(() => fromEvent(app, "window-all-closed").pipe(take(1))))
  .subscribe((event: any) => (event as IpcMainEvent).preventDefault())

// When the protocol closes, upload protocol logs to S3
zip([
  merge(fromTrigger("persisted"), fromTrigger("authFlowSuccess")).pipe(
    pluck("email")
  ),
  merge(
    fromTrigger("protocolCloseFlowSuccess"),
    fromTrigger("protocolCloseFlowSuccess")
  ),
]).subscribe(([email]: [string, ChildProcess]) => {
  uploadToS3(email).catch((err) => console.error(err))
})

// If we have have successfully authorized, close the existing windows.
// It's important to put this effect after the application closing effect.
// If not, the filters on the application closing observable don't run.
// This causes the app to close on every loginSuccess, before the protocol
// can launch.
merge(
  fromTrigger("protocolLaunchFlowSuccess"),
  fromTrigger("authFlowSuccess")
).subscribe(() => {
  closeWindows()
  hideAppDock()
  createTray()
})

fromTrigger("windowCreated").subscribe(() => showAppDock())

// If the update is downloaded, quit the app and install the update
fromTrigger("updateDownloaded").subscribe(() => {
  autoUpdater.quitAndInstall()
})

fromTrigger("updateAvailable").subscribe(() => {
  closeWindows()
  createUpdateWindow()
  autoUpdater.downloadUpdate().catch((err) => console.error(err))
})

zip(
  merge(
    fromTrigger("protocolCloseFlowSuccess"),
    fromTrigger("protocolCloseFlowFailure")
  ),
  merge(
    fromTrigger("containerFlowSuccess").pipe(mapTo(true)),
    fromTrigger("containerFlowFailure").pipe(mapTo(false))
  )
).subscribe(([, success]: [any, boolean]) => {
  if (success) app.quit()
  destroyTray()
})

merge(fromTrigger("signoutAction"), fromTrigger("relaunchAction")).subscribe(
  () => {
    persistClear()
    session.fromPartition("auth0").clearStorageData()
    app.relaunch()
    app.exit()
  }
)
