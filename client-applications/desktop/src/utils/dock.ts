/**
 * Copyright Fractal Computers, Inc. 2021
 * @file dock.ts
 * @brief This file contains functions to manipulate the MacOS app dock image.
 */

import { app } from "electron"

export const showAppDock = () => {
  // Regular activation policy has an app dock
  app?.setActivationPolicy?.("regular")
}

export const hideAppDock = () => {
  // Accessory activation policy does not have an app dock
  app?.setActivationPolicy?.("accessory")
}
