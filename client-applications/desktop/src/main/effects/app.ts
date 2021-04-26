/**
 * Copyright Fractal Computers, Inc. 2021
 * @file app.ts
 * @brief This file contains subscriptions to Electron app event emitters observables.
 */

import { app } from "electron"
import { autoUpdater } from "electron-updater"
import EventEmitter from "events"
import { fromEvent, merge, zip, combineLatest } from "rxjs"

import {
  eventUpdateAvailable,
  eventUpdateDownloaded,
} from "@app/main/events/autoupdate"
import { eventAppReady, eventWindowCreated } from "@app/main/events/app"

import { takeUntil, take, concatMap } from "rxjs/operators"
import {
  closeWindows,
  createAuthWindow,
  createUpdateWindow,
  showAppDock,
  hideAppDock,
} from "@app/utils/windows"
import { loginSuccess } from "@app/main/observables/login"
import { signupSuccess } from "@app/main/observables/signup"
import {
  protocolLaunchProcess,
  protocolCloseRequest,
} from "@app/main/observables/protocol"
import { errorWindowRequest } from "@app/main/observables/error"
import { autoUpdateAvailable } from "@app/main/observables/autoupdate"
import {
  userEmail,
  userAccessToken,
  userConfigToken,
} from "@app/main/observables/user"
import { uploadToS3 } from "@app/utils/logging"
import env from "@app/utils/env"
import { FractalCIEnvironment } from "@app/utils/config"


// appReady only fires once, at the launch of the application.
// We use takeUntil to make sure that the auth window only fires when
// we have all of [userEmail, userAccessToken, userConfigToken]. If we
// don't have all three, we clear them all and force the user to log in again.
eventAppReady
  .pipe(takeUntil(zip(userEmail, userAccessToken, userConfigToken)))
  .subscribe(() => createAuthWindow((win: any) => win.show()))

eventAppReady.pipe(take(1)).subscribe(() => {
  // We want to manually control when we download the update via autoUpdater.quitAndInstall(),
  // so we need to set autoDownload = false
  autoUpdater.autoDownload = false
  // In dev and staging, the file containing the version is called {channel}-mac.yml, so we need to set the 
  // channel down below. In prod, the file is called latest-mac.yml, which channel defaults to, so
  // we don't need to set it.
  switch(env.PACKAGED_ENV) {
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
  protocolLaunchProcess,
  loginSuccess,
  signupSuccess,
  errorWindowRequest,
  eventUpdateAvailable
)
  .pipe(
    concatMap(() =>
      fromEvent(app as EventEmitter, "window-all-closed").pipe(take(1))
    )
  )
  .subscribe((event) => event.preventDefault())

// When the protocol closes, upload protocol logs to S3
combineLatest([userEmail, protocolCloseRequest]).subscribe(([email, _]) => {
  uploadToS3(email)
    .then(() => app.quit())
    .catch((err) => {
      console.error(err)
      app.quit()
    })
})

// If we have have successfully authorized, close the existing windows.
// It's important to put this effect after the application closing effect.
// If not, the filters on the application closing observable don't run.
// This causes the app to close on every loginSuccess, before the protocol
// can launch.
merge(protocolLaunchProcess, loginSuccess, signupSuccess)
  .pipe(take(1))
  .subscribe(() => {
    closeWindows()
    hideAppDock()
  })

// If the update is downloaded, quit the app and install the update

eventUpdateDownloaded.subscribe(() => {
  autoUpdater.quitAndInstall()
})

autoUpdateAvailable.subscribe(() => {
  closeWindows()
  createUpdateWindow((win: any) => win.show())
  autoUpdater.downloadUpdate().catch((err) => console.error(err))
})

eventWindowCreated.subscribe(() => showAppDock())
