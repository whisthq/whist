/**
 * Copyright Fractal Computers, Inc. 2021
 * @file app.ts
 * @brief This file contains subscriptions to Electron app event emitters observables.
 */

import { app } from "electron"
import { autoUpdater } from "electron-updater"
import { eventUpdateDownloaded } from "@app/main/events/autoupdate"

import { eventAppReady, eventAppQuit } from "@app/main/events/app"
import { merge, race } from "rxjs"
import {
    closeWindows,
    createAuthWindow,
    createUpdateWindow,
} from "@app/utils/windows"
import { loginSuccess } from "@app/main/observables/login"
import { signupSuccess } from "@app/main/observables/signup"
import {
    autoUpdateAvailable,
    autoUpdateNotAvailable,
} from "@app/main/observables/autoupdate"

// Window opening
// appReady only fires once, at the launch of the application.
eventAppReady.subscribe(() => createAuthWindow((win: any) => win.show()))

// Window closing
// If we have have successfully authorized, close the existing windows.
merge(loginSuccess, signupSuccess).subscribe(() => closeWindows())

// Application closing
eventAppReady.subscribe(() => app.quit())
eventAppQuit.subscribe(() => app.quit())

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
