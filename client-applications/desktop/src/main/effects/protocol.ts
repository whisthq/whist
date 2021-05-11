/**
 * Copyright Fractal Computers, Inc. 2021
 * @file app.ts
 * @brief This file contains subscriptions to Observables related to protocol launching.
 */
import { zip, merge } from "rxjs"

import { protocolStreamInfo, protocolStreamKill } from "@app/utils/protocol"
import { protocolLaunchSuccess } from "@app/main/observables/protocol"
import { containerPollingSuccess } from "@app/main/observables/container"
import {
  containerInfoIP,
  containerInfoPorts,
  containerInfoSecretKey,
} from "@app/utils/container"
import { quitAction, signoutAction } from "@app/main/events/actions"
import { errorWindowRequest } from "@app/main/observables/error"

// The current implementation of the protocol process shows its own loading
// screen while a container is created and configured. To do this, we need it
// the protocol to start and appear before its mandatory arguments are available.
//
// We solve this streaming the ip, secret_key, and ports info to the protocol
// they become available from when a successful container status response.
zip(protocolLaunchSuccess, containerPollingSuccess).subscribe(
  ([protocol, response]) =>
    protocolStreamInfo(protocol, {
      ip: containerInfoIP(response),
      secret_key: containerInfoSecretKey(response),
      ports: containerInfoPorts(response),
    })
)

// If we have an error, close the protocol. We expect that an effect elsewhere
// this application will take care of showing an appropriate error message.
zip(
  protocolLaunchSuccess,
  merge(signoutAction, quitAction, errorWindowRequest)
).subscribe(([protocol]) => protocolStreamKill(protocol))
