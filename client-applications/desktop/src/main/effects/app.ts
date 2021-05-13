/**
 * Copyright Fractal Computers, Inc. 2021
 * @file app.ts
 * @brief This file contains subscriptions to Electron app event emitters observables.
 */

import { app, IpcMainEvent } from "electron"
import path from "path"
import { autoUpdater } from "electron-updater"
import { fromEvent, merge, zip } from "rxjs"
import { take, concatMap, mapTo } from "rxjs/operators"

import {
  closeWindows,
  createAuthWindow,
  createUpdateWindow,
  showAppDock,
  hideAppDock,
  createErrorWindow,
} from "@app/main/utils/window"
import { createTray } from "@app/main/utils/tray"
import { signout } from "@app/main/triggers/renderer"
import { uploadToS3 } from "@app/utils/logging"
import env from "@app/utils/env"
import { FractalCIEnvironment } from "@app/config/environment"
import { fromTrigger } from "@app/main/utils/flows"
import config from "@app/config/environment"

// Set custom app data folder based on environment
fromTrigger("appReady").subscribe(() => {
  const { deployEnv } = config
  const appPath = app.getPath("userData")
  const newPath = path.join(appPath, deployEnv)
  app.setPath("userData", newPath)
})

// Check for autoupdate. electron-updater will error if we look for an update more
// than once, so we take(1) just to be extra careful
fromTrigger("appReady")
  .pipe(take(1))
  .subscribe(async () => {
    // We want to manually control when we download the update via autoUpdater.quitAndInstall(),
    // so we need to set autoDownload = false
    autoUpdater.autoDownload = false
    // In dev and staging, the file containing the version is called {channel}-mac.yml, so we need to set the
    // channel down below. In prod, the file is called latest-mac.yml, which channel defaults to, so
    // we don't need to set it.
    switch (env.PACKAGED_ENV) {
      case FractalCIEnvironment.STAGING:
        autoUpdater.channel = "staging-rc"
        break
      case FractalCIEnvironment.DEVELOPMENT:
        autoUpdater.channel = "dev-rc"
        break
      default:
        break
    }

    // This is what looks for a latest.yml file in the S3 bucket in electron-builder.config.js,
    // and fires an update if the current version is less than the version in latest.yml
    await autoUpdater.checkForUpdatesAndNotify()
  })

fromTrigger("appReady").subscribe(() => createAuthWindow())

// By default, the window-all-closed Electron event will cause the application
// to close. We don't want this behavior for certain observables. For example,
// when the protocol launches, we close all the windows, but we don't want the app
// to quit.
merge(
  fromTrigger("protocolLaunchFlowSuccess"),
  fromTrigger("loginFlowSuccess"),
  fromTrigger("signupFlowSuccess"),
  fromTrigger("errorWindow"),
  fromTrigger("updateAvailable")
)
  .pipe(concatMap(() => fromEvent(app, "window-all-closed").pipe(take(1))))
  .subscribe((event: any) => (event as IpcMainEvent).preventDefault())

// When the protocol closes, upload protocol logs to S3
// combineLatest([
//   userEmail,
//   merge(protocolCloseFlow.success,
// ]).subscribe(([email]: [string, ChildProcess]) => {
//   uploadToS3(email).catch((err) => console.error(err))
// })

// If we have have successfully authorized, close the existing windows.
// It's important to put this effect after the application closing effect.
// If not, the filters on the application closing observable don't run.
// This causes the app to close on every loginSuccess, before the protocol
// can launch.

merge(
  fromTrigger("protocolLaunchFlowSuccess"),
  fromTrigger("loginFlowSuccess"),
  fromTrigger("signupFlowSuccess")
).subscribe(() => {
  closeWindows()
  hideAppDock()
  // createTray(eventActionTypes)
})

// If the update is downloaded, quit the app and install the update

fromTrigger("updateDownloaded").subscribe(() => {
  autoUpdater.quitAndInstall()
})

fromTrigger("updateAvailable").subscribe(async () => {
  closeWindows()
  createUpdateWindow()
  await autoUpdater.downloadUpdate()
})

fromTrigger("windowCreated").subscribe(() => showAppDock())

zip(
  merge(
    fromTrigger("protocolCloseFlowSuccess"),
    fromTrigger("protocolCloseFlowFailure")
  ),
  fromTrigger("protocolLaunchSuccess").pipe(mapTo(true))
).subscribe(([, success]: [any, boolean]) => {
  if (success) app.quit()
})

fromTrigger("signout").subscribe(() => {
  app.relaunch()
  app.exit()
})
