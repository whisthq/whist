/**
 * Copyright Fractal Computers, Inc. 2021
 * @file app.ts
 * @brief This file contains subscriptions to Electron app event emitters observables.
 */

import { app } from 'electron'
import { autoUpdater } from 'electron-updater'
import { eventUpdateDownloaded } from '@app/main/events/autoupdate'

import {
  eventAppReady,
  eventWindowsAllClosed,
  eventWindowCreated
} from '@app/main/events/app'
import { merge, race, zip } from 'rxjs'
import { takeUntil } from 'rxjs/operators'
import {
  closeWindows,
  createAuthWindow,
  createUpdateWindow,
  showAppDock
} from '@app/utils/windows'
import { loginSuccess } from '@app/main/observables/login'
import { signupSuccess } from '@app/main/observables/signup'
import { errorWindowRequest } from '@app/main/observables/error'
import {
  autoUpdateAvailable,
  autoUpdateNotAvailable
} from '@app/main/observables/autoupdate'
import {
  userEmail,
  userAccessToken,
  userConfigToken
} from '@app/main/observables/user'
import { protocolCloseSuccess } from '@app/main/observables/protocol'

// appReady only fires once, at the launch of the application.
// We use takeUntil to make sure that the auth window only fires when
// we have all of [userEmail, userAccessToken, userConfigToken]. If we
// don't have all three, we clear them all and force the user to log in again.

eventAppReady
  .pipe(takeUntil(zip(userEmail, userAccessToken, userConfigToken)))
  .subscribe(() => createAuthWindow((win: any) => win.show()))

// Closing all the windows should simply quit the application entirely.
// This event fires both when the user intentionally closes windows, and
// also when windows automatically close (protocol launch, errors, etc.)
// It's important that we don't fire this subscription on events that aren't
// supposed to close the application, so we use takeUntil to listen for those.

eventWindowsAllClosed
  .pipe(takeUntil(merge(loginSuccess, signupSuccess, errorWindowRequest)))
  .subscribe(() => app.quit())

// If we have have successfully authorized, close the existing windows.
// It's important to put this effect after the application closing effect.
// If not, the filters on the application closing observable don't run.
// This causes the app to close on every loginSuccess, before the protocol
// can launch.

merge(loginSuccess, signupSuccess).subscribe(() => closeWindows())

// If the update is downloaded, quit the app and install the update

eventUpdateDownloaded.subscribe(() => autoUpdater.quitAndInstall())

race(autoUpdateAvailable, autoUpdateNotAvailable).subscribe(
  (available: boolean) => {
    if (available) {
      closeWindows()
      createUpdateWindow((win: any) => win.show())
      autoUpdater.downloadUpdate().catch((err) => console.error(err))
    }
  }
)

eventWindowCreated.subscribe(() => showAppDock())

// When the protocol closes, we'll close the entire application.
protocolCloseSuccess.subscribe(() => app?.quit())
