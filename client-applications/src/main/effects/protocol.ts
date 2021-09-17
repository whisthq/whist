/**
 * Copyright Fractal Computers, Inc. 2021
 * @file app.ts
 * @brief This file contains subscriptions to Observables related to protocol launching.
 */

import { app } from "electron"
import { fromTrigger } from "@app/utils/flows"
import { protocolStreamInfo, childProcess } from "@app/utils/protocol"
import {
  closeElectronWindows,
  createProtocolWindow,
  getElectronWindows,
} from "@app/utils/windows"

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
        .catch((err) => console.error(err))
    } else {
      protocolStreamInfo(info)
    }
    closeElectronWindows(getElectronWindows())
    app?.dock?.hide()
  }
)
