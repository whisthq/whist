/**
 * Copyright Fractal Computers, Inc. 2021
 * @file app.ts
 * @brief This file contains subscriptions to error Observables.
 */

import { app } from "electron"
import { closeWindows } from "@app/utils/windows"

import {
    errorRelaunchRequest,
    errorWindowRequest,
} from "@app/main/observables/error"

errorRelaunchRequest.subscribe((req) => {
    if (req) {
        app.relaunch()
        app.exit()
    }
})

// Error windows
errorWindowRequest.subscribe((windowFunction) => {
    closeWindows(), windowFunction()
})
