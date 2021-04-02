/**
 * Copyright Fractal Computers, Inc. 2021
 * @file app.ts
 * @brief This file contains subscriptions to Observables related to protocol launching.
*/


import { zip } from "rxjs"
import { protocolStreamInfo, protocolStreamKill } from "@app/utils/protocol"
import {
    protocolLaunchRequest,
    protocolLaunchSuccess,
    protocolLaunchFailure,
} from "@app/main/observables/protocol"

// Streaming information to protocol
// Stream the ip, secret_key, and ports info to the protocol when we
// when we receive a successful container status response.
zip(
    protocolLaunchRequest,
    protocolLaunchSuccess
).subscribe(([protocol, info]) => protocolStreamInfo(protocol, info))

// Protocol closing
// If we have an error, close the protocol.
zip(
    protocolLaunchRequest,
    protocolLaunchFailure
).subscribe(([protocol, _error]) => protocolStreamKill(protocol))
