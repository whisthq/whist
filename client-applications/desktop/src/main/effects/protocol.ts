/**
 * Copyright Fractal Computers, Inc. 2021
 * @file app.ts
 * @brief This file contains subscriptions to Observables related to protocol launching.
 */
import { zip, merge } from "rxjs"
import { ChildProcess } from "child_process"

import {
  protocolStreamInfo,
  protocolStreamKill,
} from "@app/main/utils/protocol"
import {
  containerPollingIP,
  containerPollingPorts,
  containerPollingSecretKey,
} from "@app/main/utils/containerPolling"
import { fromTrigger } from "@app/main/utils/flows"

// The current implementation of the protocol process shows its own loading
// screen while a container is created and configured. To do this, we need it
// the protocol to start and appear before its mandatory arguments are available.
//
// We solve this streaming the ip, secret_key, and ports info to the protocol
// they become available from when a successful container status response.

zip(
  fromTrigger("protocolLaunchFlowSuccess"),
  fromTrigger("containerFlowSuccess")
).subscribe(([launch, response]: [{protocol: ChildProcess}, any]) =>
  protocolStreamInfo(launch.protocol, {
    containerIP: containerPollingIP(response),
    containerSecret: containerPollingSecretKey(response),
    containerPorts: containerPollingPorts(response),
  })
)

// If we have an error, close the protocol. We expect that an effect elsewhere
// this application will take care of showing an appropriate error message.
zip(
  fromTrigger("protocolLaunchFlowSuccess"),
  merge(
    fromTrigger("signout"),
    fromTrigger("quit"),
    fromTrigger("errorWindow")
  )
).subscribe(([protocol]: [ChildProcess]) => protocolStreamKill(protocol))
