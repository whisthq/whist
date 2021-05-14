/**
 * Copyright Fractal Computers, Inc. 2021
 * @file app.ts
 * @brief This file contains subscriptions to Electron app event emitters observables.
 */

import { app } from "electron"
import { autoUpdater } from "electron-updater"
import { fromEvent, merge, zip, combineLatest } from "rxjs"
import { mapTo, takeUntil, take, concatMap } from "rxjs/operators"
import { ChildProcess } from "child_process"

import EventEmitter from "events"
import {
  eventUpdateAvailable,
  eventUpdateDownloaded,
} from "@app/main/events/autoupdate"
import { eventAppReady, eventWindowCreated } from "@app/main/events/app"
import { eventActionTypes } from "@app/main/events/tray"
import { authEvents } from "@app/main/events/auth"
import {
  closeWindows,
  createAuthWindow,
  createUpdateWindow,
  showAppDock,
  hideAppDock,
} from "@app/utils/windows"
import { createTray } from "@app/utils/tray"
import { signoutAction } from "@app/main/events/actions"
import {
  refreshTokens,
  generateRandomConfigToken,
  storeConfigToken,
} from "@app/utils/auth"

import {
  protocolLaunchProcess,
  protocolCloseRequest,
  protocolLaunchSuccess,
  protocolLaunchFailure,
} from "@app/main/observables/protocol"
import { errorWindowRequest } from "@app/main/observables/error"
import {
  userSub,
  userConfigToken,
  userRefreshToken,
} from "@app/main/observables/user"
import { authSuccess } from "@app/main/observables/auth"
import { uploadToS3 } from "@app/utils/logging"
import { clearKeys } from "@app/utils/persist"
import { config, FractalEnvironments } from "@app/config/environment"

// appReady only fires once, at the launch of the application.
// We use takeUntil to make sure that the auth window only fires when
// we have all of [userSub, userAccessToken, userConfigToken]. If we
// don't have all three, we clear them all and force the user to log in again.
eventAppReady.pipe(takeUntil(authSuccess)).subscribe(() => {
  createAuthWindow((win: any) => win.show())
})

eventAppReady.pipe(take(1)).subscribe(() => {
  // We want to manually control when we download the update via autoUpdater.quitAndInstall(),
  // so we need to set autoDownload = false
  autoUpdater.autoDownload = false
  // In dev and staging, the file containing the version is called {channel}-mac.yml, so we need to set the
  // channel down below. In prod, the file is called latest-mac.yml, which channel defaults to, so
  // we don't need to set it.
  switch (config.appEnvironment) {
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

// By default, the window-all-closed Electron event will cause the application
// to close. We don't want this behavior for certain observables. For example,
// when the protocol launches, we close all the windows, but we don't want the app
// to quit.

merge(protocolLaunchProcess, errorWindowRequest, eventUpdateAvailable)
  .pipe(
    concatMap(() =>
      fromEvent(app as EventEmitter, "window-all-closed").pipe(take(1))
    )
  )
  .subscribe((event: any) => event.preventDefault())

// If we have have successfully authorized, close the existing windows.
// It's important to put this effect after the application closing effect.
// If not, the filters on the application closing observable don't run.
// This causes the app to close on every loginSuccess, before the protocol
// can launch.
merge(protocolLaunchProcess, authSuccess)
  .pipe(take(1))
  .subscribe(() => {
    closeWindows()
    hideAppDock()
    createTray(eventActionTypes)
  })

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

// When the protocol closes, upload protocol logs to S3
combineLatest([
  userSub,
  protocolCloseRequest,
  merge(
    protocolLaunchSuccess.pipe(mapTo(true)),
    protocolLaunchFailure.pipe(mapTo(false))
  ),
]).subscribe(([email, , success]: [string, ChildProcess, boolean]) => {
  uploadToS3(email)
    .then(() => {
      if (success) app.exit()
    })
    .catch((err) => {
      console.error(err)
      app.exit()
    })
})

quitAction.subscribe(() => {
  app.exit()
})

signoutAction.subscribe(() => {
  clearKeys("refreshToken", "accessToken")
  app.relaunch()
  app.exit()
})

// If no valid access token exists, we regenerate one
userRefreshToken
  .pipe(takeUntil(authSuccess))
  // eslint-disable-next-line @typescript-eslint/no-misused-promises
  .subscribe(async (refreshToken: string) => {
    const data = await refreshTokens(refreshToken)
    authEvents.refreshTokens(data)
  })

// If no config token, randomly generate one and store it
userRefreshToken.pipe(takeUntil(userConfigToken)).subscribe(() => {
  const configToken = generateRandomConfigToken()
  // eslint-disable-next-line no-void
  void storeConfigToken(configToken)
})
