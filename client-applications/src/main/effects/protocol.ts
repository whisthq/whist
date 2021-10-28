/**
 * Copyright 2021 Fractal Computers, Inc., dba Whist
 * @file app.ts
 * @brief This file contains subscriptions to Observables related to protocol launching.
 */

import Sentry from "@sentry/electron"

import { fromTrigger } from "@app/utils/flows"
import { protocolStreamInfo, childProcess } from "@app/utils/protocol"
import { createProtocolWindow } from "@app/utils/windows"
import { persistGet, persist } from "@app/utils/persist"

// The current implementation of the protocol process shows its own loading
// screen while a mandelbox is created and configured. To do this, we need it
// the protocol to start and appear before its mandatory arguments are available.

// We solve this streaming the ip, secret_key, and ports info to the protocol
// they become available from when a successful mandelbox status response.

fromTrigger("mandelboxFlowSuccess").subscribe(
  (info: {
    mandelboxIP: string
    mandelboxSecret: string
    mandelboxPorts: {
      port_32262: number
      port_32263: number
      port_32273: number
    }
  }) => {
    if (childProcess === undefined) {
      createProtocolWindow()
        .then(() => {
          protocolStreamInfo(info)
        })
        .catch((err) => Sentry.captureException(err))
    } else {
      protocolStreamInfo(info)
    }
  }
)

fromTrigger("trayRestoreSessionAction").subscribe(() => {
  const restore =
    (persistGet("restoreLastChromeSession", "data") ?? "true") === "true"
  persist("restoreLastChromeSession", !restore, "data")
})
