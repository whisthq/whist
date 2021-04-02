// This file is home to observables that manage launching the protocol process
// on the user's machine. They are distinct from the observables that
// communicate with the webserver to manage container creation.
//
// Many of these observables emit the protocol ChildProcess object, which
// carries important data about the state of the protocol process.

import { protocolLaunch } from "@app/utils/protocol"
import {
    containerInfoIP,
    containerInfoPorts,
    containerInfoSecretKey,
} from "@app/utils/container"
import {
    containerAssignRequest,
    containerAssignSuccess,
    containerAssignFailure,
} from "@app/main/container"
import { merge, zip, of, fromEvent } from "rxjs"
import { mapTo, map, filter, share, exhaustMap, mergeMap } from "rxjs/operators"

export const protocolLaunchRequest = containerAssignRequest.pipe(
    exhaustMap(() => of(protocolLaunch())),
    share()
)
export const protocolLaunchLoading = protocolLaunchRequest.pipe(
    exhaustMap(() => merge(of(true), containerAssignSuccess.pipe(mapTo(false))))
)

export const protocolLaunchSuccess = containerAssignSuccess.pipe(
    map((res) => ({
        ip: containerInfoIP(res),
        secret_key: containerInfoSecretKey(res),
        ports: containerInfoPorts(res),
    }))
)

export const protocolLaunchFailure = containerAssignFailure

export const protocolCloseRequest = protocolLaunchRequest.pipe(
    mergeMap((protocol) => zip(of(protocol), fromEvent(protocol, "close")))
)

export const protocolCloseFailure = protocolCloseRequest.pipe(
    filter(([protocol]) => protocol.killed)
)
