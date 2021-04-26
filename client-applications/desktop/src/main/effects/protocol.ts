/**
 * Copyright Fractal Computers, Inc. 2021
 * @file app.ts
 * @brief This file contains subscriptions to Observables related to protocol launching.
 */
import { zip } from "rxjs"

import { protocolStreamInfo, protocolStreamKill } from "@app/utils/protocol"
import {
  protocolLaunchProcess,
  protocolLaunchSuccess,
  protocolLaunchFailure,
} from "@app/main/observables/protocol"

// The current implementation of the protocol process shows its own loading
// screen while a container is created and configured. To do this, we need it
// the protocol to start and appear before its mandatory arguments are available.
//
// We solve this streaming the ip, secret_key, and ports info to the protocol
// they become available from when a successful container status response.
zip(
  protocolLaunchProcess,
  protocolLaunchSuccess
).subscribe(([protocol, info]) => protocolStreamInfo(protocol, info))

// If we have an error, close the protocol. We expect that an effect elsewhere
// this application will take care of showing an appropriate error message.
zip(
  protocolLaunchProcess,
  protocolLaunchFailure
).subscribe(([protocol, _error]) => protocolStreamKill(protocol))
