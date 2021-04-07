/**
 * Copyright Fractal Computers, Inc. 2021
 * @file app.ts
 * @brief This file contains subscriptions to Electron app event emitters observables.
 */

import { app } from 'electron'
import { autoUpdater } from 'electron-updater'
import { eventUpdateDownloaded } from '@app/main/events/autoupdate'

import { eventAppReady, eventWindowsAllClosed } from '@app/main/events/app'
import { merge, race } from 'rxjs'
import { takeUntil } from 'rxjs/operators'
import {
  closeWindows,
  createAuthWindow,
  createUpdateWindow
} from '@app/utils/windows'
import { loginSuccess } from '@app/main/observables/login'
import { signupSuccess } from '@app/main/observables/signup'
import { errorWindowRequest } from '@app/main/observables/error'
import {
  autoUpdateAvailable,
  autoUpdateNotAvailable
} from '@app/main/observables/autoupdate'

// Window opening
// appReady only fires once, at the launch of the application.
eventAppReady.subscribe(() => createAuthWindow((win: any) => win.show()))

// Application closing
eventWindowsAllClosed
  .pipe(takeUntil(merge(loginSuccess, signupSuccess, errorWindowRequest)))
  .subscribe(() => app.quit())

// Window closing
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
      autoUpdater.downloadUpdate()
      createUpdateWindow((win: any) => win.show())
    }
  }
)
