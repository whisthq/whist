/**
 * Copyright Fractal Computers, Inc. 2021
 * @file app.ts
 * @brief This file contains subscriptions to Electron app event emitters observables.
 */

import { session } from "electron"
import { autoUpdater } from "electron-updater"
import { take } from "rxjs/operators"
import { merge } from "rxjs"

import { AWSRegion } from "@app/@types/aws"
import {
  createAuthWindow,
  createSignoutWindow,
  createLoadingWindow,
  relaunch,
  createPaymentWindow,
  createExitTypeform,
  createBugTypeform,
  createOnboardingTypeform,
  closeAllWindows,
  createProtocolWindow,
} from "@app/utils/windows"
import { createTray, createMenu } from "@app/utils/tray"
import { fromTrigger } from "@app/utils/flows"
import { emitCache, persist, persistClear, store } from "@app/utils/persist"
import { showAppDock, hideAppDock } from "@app/utils/dock"
import { fromSignal } from "@app/utils/observables"

fromTrigger("appReady").subscribe(() => {
  emitCache()
  createTray(createMenu(false))
})

fromTrigger("notPersisted").subscribe(() => {
  showAppDock()
  createAuthWindow()
})

// If we have have successfully authorized, close the existing windows.
// It's important to put this effect after the application closing effect.
// If not, the filters on the application closing observable don't run.
// This causes the app to close on every loginSuccess, before the protocol
// can launch.
merge(fromTrigger("configFlowSuccess")).subscribe(
  (x: { userEmail: string }) => {
    // On MacOS, hide the app dock when the protocol is open
    hideAppDock()
    // Create the protocol loading window
    createLoadingWindow()
    createProtocolWindow().catch((err) => console.error(err))
    // Present the tray (top right corner of screen)
    createTray(createMenu(true, x.userEmail))
  }
)

// If an update is available, show the update window and download the update
fromTrigger("updateAvailable")
  .pipe(take(1))
  .subscribe(() => {
    autoUpdater.downloadUpdate().catch((err) => console.error(err))
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
      .catch((err) => console.error(err))
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

fromTrigger("showSignoutWindow").subscribe(() => {
  createSignoutWindow()
})

fromTrigger("trayFeedbackAction").subscribe(() => {
  createExitTypeform()
})

fromTrigger("trayBugAction").subscribe(() => {
  createBugTypeform()
})

fromSignal(fromTrigger("onboarded"), fromTrigger("authFlowSuccess")).subscribe(
  (onboarded: boolean) => {
    if (!onboarded) createOnboardingTypeform()
  }
)

// eslint-disable-next-line @typescript-eslint/no-misused-promises
fromTrigger("showPaymentWindow").subscribe(() => {
  const accessToken = (store.get("auth.accessToken") ?? "") as string
  const refreshToken = (store.get("auth.refreshToken") ?? "") as string
  createPaymentWindow({
    accessToken,
    refreshToken,
  }).catch((err) => console.error(err))
})

// eslint-disable-next-line @typescript-eslint/no-misused-promises
fromTrigger("checkPaymentFlowFailure").subscribe(
  ({ accessToken, refreshToken }) => {
    createPaymentWindow({
      accessToken,
      refreshToken,
    }).catch((err) => console.error(err))
  }
)

fromTrigger("exitTypeformSubmitted").subscribe(() => {
  persist("exitTypeformSubmitted", true, "data")
  closeAllWindows()
})

fromTrigger("onboardingTypeformSubmitted").subscribe(() => {
  persist("onboardingTypeformSubmitted", true, "data")
})
