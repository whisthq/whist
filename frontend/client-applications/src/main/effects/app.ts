/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file app.ts
 * @brief This file contains effects that deal with the Electron app module
 */

import { app } from "electron"
import { ChildProcess } from "child_process"

import { fromTrigger } from "@app/main/utils/flows"
import { WhistTrigger } from "@app/constants/triggers"
import { emitOnSignal } from "@app/main/utils/observables"
import { destroyProtocol } from "@app/main/utils/protocol"

// This prevents duplicate apps from opening when the user clicks on the icon
// multiple times
fromTrigger(WhistTrigger.appReady).subscribe(() => {
  app.requestSingleInstanceLock()

  app.on("second-instance", (e) => {
    e.preventDefault()
  })
})

emitOnSignal(
  fromTrigger(WhistTrigger.protocol),
  fromTrigger(WhistTrigger.appWillQuit)
).subscribe((p: ChildProcess) => {
  destroyProtocol(p)
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
