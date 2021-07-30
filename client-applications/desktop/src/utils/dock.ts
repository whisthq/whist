/**
 * Copyright Fractal Computers, Inc. 2021
 * @file dock.ts
 * @brief This file contains functions to manipulate the MacOS app dock image.
 */

import { app } from "electron"

export const showAppDock = () => {
    // In case it's hidden, show the app dock
    app?.dock?.show().catch((err) => console.error(err))
}

export const hideAppDock = () => {
    // Hide the app dock just in case
    app?.dock?.hide()
}
