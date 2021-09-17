/**
 * Copyright Fractal Computers, Inc. 2021
 * @file dock.ts
 * @brief This file contains functions to manipulate the MacOS app dock image.
 */

import { app } from "electron"

export const showAppDock = () => {
  // Regular activation policy has an app dock
  app?.dock?.show()
}

export const hideAppDock = () => {
  // Accessory activation policy does not have an app dock
  app?.dock?.hide()
}

export const bounceAppDock = (bounce: boolean) => {
  // Critical means the dock keeps bouncing, information means the dock bounces once
  bounce ? app?.dock?.bounce("critical") : app?.dock?.hide()
}
