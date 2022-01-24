/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file app.ts
 * @brief This file contains effects that deal with the Electron app module
 */

import { app } from "electron"

import { fromTrigger } from "@app/main/utils/flows"
import { WhistTrigger } from "@app/constants/triggers"

// This prevents duplicate apps from opening when the user clicks on the icon
// multiple times
fromTrigger(WhistTrigger.appReady).subscribe(() => {
  app.requestSingleInstanceLock()

  app.on("second-instance", (e) => {
    e.preventDefault()
  })
})

// If the user requests/unrequests Whist as their default browser
fromTrigger(WhistTrigger.setDefaultBrowser).subscribe(
  (body: { default: boolean }) => {
    if (body.default) {
      app.setAsDefaultProtocolClient("http")
      app.setAsDefaultProtocolClient("https")
    } else {
      app.removeAsDefaultProtocolClient("http")
      app.removeAsDefaultProtocolClient("https")
    }
  }
)
