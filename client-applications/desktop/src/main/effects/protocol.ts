/**
 * Copyright Fractal Computers, Inc. 2021
 * @file app.ts
 * @brief This file contains subscriptions to Observables related to protocol launching.
 */

import { protocolStreamInfo } from "@app/utils/protocol"
import { fromTrigger } from "@app/utils/flows"

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
        protocolStreamInfo(info)
    }
)
