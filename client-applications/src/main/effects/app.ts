/**
 * Copyright 2021 Fractal Computers, Inc., dba Whist
 * @file app.ts
 * @brief This file contains subscriptions to Electron app event emitters observables.
 */

import { app, session } from "electron"
import { autoUpdater } from "electron-updater"
import { take } from "rxjs/operators"
import Sentry from "@sentry/electron"
import { zip } from "rxjs"

import { AWSRegion } from "@app/@types/aws"
import {
  createSignoutWindow,
  relaunch,
  createPaymentWindow,
  createBugTypeform,
  createOnboardingTypeform,
} from "@app/utils/windows"
import { createTray, createMenu } from "@app/utils/tray"
import { fromTrigger } from "@app/utils/flows"
import { persist, persistGet, persistClear, store } from "@app/utils/persist"
import { withAppReady } from "@app/utils/observables"
import { startupNotification } from "@app/utils/notification"
import { accessToken } from "@fractal/core-ts"

fromTrigger("appReady").subscribe(() => {
  createTray(createMenu(false))
})

// If we have have successfully authorized, close the existing windows.
// It's important to put this effect after the application closing effect.
// If not, the filters on the application closing observable don't run.
// This causes the app to close on every loginSuccess, before the protocol
// can launch.
withAppReady(fromTrigger("configFlowSuccess")).subscribe(
  (x: { userEmail: string }) => {
    // Show notification
    startupNotification()?.show()
    // Present the tray (top right corner of screen)
    createTray(createMenu(true, x.userEmail))
  }
)

// If an update is available, show the update window and download the update
fromTrigger("updateAvailable")
  .pipe(take(1))
  .subscribe(() => {
    autoUpdater.downloadUpdate().catch((err) => Sentry.captureException(err))
  })

// On signout or relaunch, clear the cache (so the user can log in again) and restart
// the app
fromTrigger("clearCacheAction").subscribe(
  (payload: { clearConfig: boolean }) => {
    persistClear(
      [
        ...["accessToken", "refreshToken", "userEmail"],
        ...(payload.clearConfig ? ["configToken"] : []),
      ],
      "auth"
    )
    // Clear the Auth0 cache. In window.ts, we tell Auth0 to store session info in
    // a partition called "auth0", so we clear the "auth0" partition here
    session
      .fromPartition("auth0")
      .clearStorageData()
      .catch((err) => Sentry.captureException(err))
    // Restart the app
    relaunch()
  }
)

// If an admin selects a region, relaunch the app with the selected region passed
// into argv so it can be read by flows/index.ts
fromTrigger("trayRegionAction").subscribe((region: AWSRegion) => {
  relaunch({ args: process.argv.slice(1).concat([region]) })
})

fromTrigger("relaunchAction").subscribe(() => {
  relaunch()
})

withAppReady(fromTrigger("showSignoutWindow")).subscribe(() => {
  createSignoutWindow()
})

withAppReady(fromTrigger("trayBugAction")).subscribe(() => {
  createBugTypeform()
})

withAppReady(fromTrigger("authFlowSuccess"))
  .pipe(take(1))
  .subscribe(() => {
    const onboarded =
      (persistGet("onboardingTypeformSubmitted", "data") as boolean) ?? false
    if (!onboarded) createOnboardingTypeform()
  })

// eslint-disable-next-line @typescript-eslint/no-misused-promises
withAppReady(fromTrigger("showPaymentWindow")).subscribe(() => {
  const accessToken = (store.get("auth.accessToken") ?? "") as string
  createPaymentWindow({
    accessToken,
  }).catch((err) => Sentry.captureException(err))
})

// eslint-disable-next-line @typescript-eslint/no-misused-promises
withAppReady(fromTrigger("checkPaymentFlowFailure")).subscribe(
  ({ accessToken }: accessToken) => {
    createPaymentWindow({
      accessToken,
    }).catch((err) => Sentry.captureException(err))
  }
)

withAppReady(fromTrigger("onboardingTypeformSubmitted")).subscribe(() => {
  persist("onboardingTypeformSubmitted", true, "data")
})

fromTrigger("appReady").subscribe(() => {
  app.requestSingleInstanceLock()

  app.on("second-instance", (e) => {
    e.preventDefault()
  })
})
