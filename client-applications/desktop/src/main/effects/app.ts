/**
 * Copyright Fractal Computers, Inc. 2021
 * @file app.ts
 * @brief This file contains subscriptions to Electron app event emitters observables.
*/

import { app } from "electron"
import { eventAppReady, eventAppQuit } from "@app/main/events/app"
import { zip } from "rxjs"
import {
    closeWindows,
    createAuthWindow,
} from "@app/utils/windows"
import {
    userEmail,
    userAccessToken,
} from "@app/main/observables/user"


// Window opening
// appReady only fires once, at the launch of the application.
eventAppReady.subscribe(() => {
    createAuthWindow((win: any) => win.show())
})

// Window closing
// If we have an access token and an email, close the existing windows.
zip(userEmail, userAccessToken).subscribe(() => {
    closeWindows()
})

// Application closing
eventAppQuit.subscribe(() => {
    // On macOS it is common for applications and their menu bar
    // to stay active until the user quits explicitly with Cmd + Q
    if (process.platform !== "darwin") app.quit()
})
