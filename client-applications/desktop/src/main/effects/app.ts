/**
 * Copyright Fractal Computers, Inc. 2021
 * @file app.ts
 * @brief This file contains subscriptions to Electron app event emitters observables.
 */

import { app, IpcMainEvent } from "electron"
import { autoUpdater } from "electron-updater"
import { fromEvent, merge, zip, combineLatest } from "rxjs"
import { mapTo, takeUntil, take, concatMap } from "rxjs/operators"
import { ChildProcess } from "child_process"

import {
  eventUpdateAvailable,
  eventUpdateDownloaded,
} from "@app/main/events/autoupdate"
import { eventAppReady, eventWindowCreated } from "@app/main/events/app"
import { eventActionTypes } from "@app/main/events/tray"

import {
  closeWindows,
  createAuthWindow,
  createUpdateWindow,
  showAppDock,
  hideAppDock,
} from "@app/main/flows/error/utils"
import { createTray } from "@app/utils/tray"
import { signoutAction } from "@app/main/events/actions"
import {
  loginFlow
} from "@app/main/flows/login"
import {
  protocolLaunchFlow,
  protocolCloseFlow
} from "@app/main/flows/protocol"
import { errorWindowRequest } from "@app/main/flows/error"
import { uploadToS3 } from "@app/utils/logging"
import env from "@app/utils/env"
import { FractalCIEnvironment } from "@app/config/environment"

// appReady only fires once, at the launch of the application.
// We use takeUntil to make sure that the auth window only fires when
// we have all of [userEmail, userAccessToken, userConfigToken]. If we
// don't have all three, we clear them all and force the user to log in again.
// eventAppReady
//   .pipe(takeUntil(zip(userEmail, userAccessToken, userConfigToken)))
//   .subscribe(() => {
//     createAuthWindow((win: any) => win.show())
//   })

eventAppReady.pipe(take(1)).subscribe(() => {
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
  autoUpdater.checkForUpdatesAndNotify().catch((err) => console.error(err))
})

// By default, the window-all-closed Electron event will cause the application
// to close. We don't want this behavior for certain observables. For example,
// when the protocol launches, we close all the windows, but we don't want the app
// to quit.

merge(
  // protocolLaunchSuccess,
  // loginSuccess,
  // signupSuccess,
  errorWindowRequest,
  eventUpdateAvailable
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
// merge(protocolLaunchSuccess, loginSuccess, signupSuccess).subscribe(() => {
//   closeWindows()
//   hideAppDock()
//   createTray(eventActionTypes)
// })

// If the update is downloaded, quit the app and install the update

eventUpdateDownloaded.subscribe(() => {
  autoUpdater.quitAndInstall()
})

eventUpdateAvailable.subscribe(() => {
  closeWindows()
  createUpdateWindow((win: any) => win.show())
  autoUpdater.downloadUpdate().catch((err) => console.error(err))
})

eventWindowCreated.subscribe(() => showAppDock())

// zip(
//   merge(protocolCloseSuccess, protocolCloseFailure),
//   protocolLaunchSuccess.pipe(mapTo(true))
// ).subscribe(([, success]) => {
//   if (success) app.quit()
// })

signoutAction.subscribe(() => {
  app.relaunch()
  app.exit()
})
