/**
 * Copyright Fractal Computers, Inc. 2021
 * @file app.ts
 * @brief This file contains subscriptions to Electron app event emitters observables.
 */

import { app, session } from "electron"
import { autoUpdater } from "electron-updater"
import { take } from "rxjs/operators"
import { isEmpty, pickBy } from "lodash"

import { AWSRegion } from "@app/@types/aws"
import {
  createAuthWindow,
  createSignoutWindow,
  relaunch,
  createPaymentWindow,
  createExitTypeform,
  createBugTypeform,
  createOnboardingTypeform,
  closeAllWindows,
  closeElectronWindows,
  getElectronWindows,
} from "@app/utils/windows"
import { createTray, createMenu } from "@app/utils/tray"
import { fromTrigger } from "@app/utils/flows"
import {
  persist,
  persistGet,
  persistClear,
  store,
  persistedAuth,
} from "@app/utils/persist"
import { fromSignal } from "@app/utils/observables"
import { startupNotification } from "@app/utils/notification"

fromTrigger("appReady").subscribe(() => {
  createTray(createMenu(false))
})

fromTrigger("appReady").subscribe(() => {
  const authCache = {
    accessToken: (persistedAuth?.accessToken ?? "") as string,
    refreshToken: (persistedAuth?.refreshToken ?? "") as string,
    userEmail: (persistedAuth?.userEmail ?? "") as string,
    configToken: (persistedAuth?.configToken ?? "") as string,
  }

  if (!isEmpty(pickBy(authCache, (x) => x === ""))) {
    app?.dock?.show()
    createAuthWindow()
  }
})

// If we have have successfully authorized, close the existing windows.
// It's important to put this effect after the application closing effect.
// If not, the filters on the application closing observable don't run.
// This causes the app to close on every loginSuccess, before the protocol
// can launch.
fromSignal(fromTrigger("configFlowSuccess"), fromTrigger("appReady")).subscribe(
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

fromTrigger("authFlowSuccess")
  .pipe(take(1))
  .subscribe(() => {
    const onboarded =
      (persistGet("onboardingTypeformSubmitted", "data") as boolean) ?? false
    if (!onboarded) createOnboardingTypeform()
  })

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
  closeElectronWindows(getElectronWindows())
})

fromTrigger("appReady").subscribe(() => {
  app.requestSingleInstanceLock()

  app.on("second-instance", (e) => {
    e.preventDefault()
  })
})
