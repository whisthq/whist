// This file is home to observables that manage launching the protocol process
// on the user's machine. They are distinct from the observables that
// communicate with the webserver to manage container creation.
//
// Many of these observables emit the protocol ChildProcess object, which
// carries important data about the state of the protocol process.

import {
  zip,
  of,
  fromEvent,
  merge,
  combineLatest,
  interval,
  Observable,
} from "rxjs"
import { map, filter, share, mergeMap, throttle } from "rxjs/operators"
import { ChildProcess } from "child_process"
import { EventEmitter } from "events"

import { protocolLaunch } from "@app/utils/protocol"
import {
  containerInfoIP,
  containerInfoPorts,
  containerInfoSecretKey,
} from "@app/utils/container"
import { debugObservables, errorObservables } from "@app/utils/logging"
import {
  containerPollingSuccess,
  containerPollingFailure,
} from "@app/main/observables/container"
import {
  userEmail,
  userAccessToken,
  userConfigToken,
} from "@app/main/observables/user"
import { hostConfigFailure } from "@app/main/observables/host"
import { loadingFrom } from "@app/utils/observables"
import { formatObservable, formatChildProcess } from "@app/utils/formatters"
import { eventUpdateNotAvailable } from "@app/main/events/autoupdate"

export const protocolLaunchProcess = combineLatest([
  zip(userEmail, userAccessToken, userConfigToken),
  eventUpdateNotAvailable,
]).pipe(
  throttle(() => interval(10000)),
  map(() => protocolLaunch()),
  share()
) as Observable<ChildProcess>

export const protocolLaunchSuccess = containerPollingSuccess.pipe(
  map((res) => ({
    ip: containerInfoIP(res),
    secret_key: containerInfoSecretKey(res),
    ports: containerInfoPorts(res),
  }))
)

export const protocolLaunchFailure = merge(
  hostConfigFailure,
  containerPollingFailure
)

export const protocolLoading = loadingFrom(
  protocolLaunchProcess,
  protocolLaunchSuccess,
  protocolLaunchFailure
)

export const protocolCloseRequest = protocolLaunchProcess.pipe(
  mergeMap((protocol) =>
    zip(of(protocol), fromEvent(protocol as EventEmitter, "close"))
  ),
  map(([protocol]) => protocol)
)

export const protocolCloseFailure = protocolCloseRequest.pipe(
  filter((protocol: ChildProcess) => protocol.killed)
)

export const protocolCloseSuccess = protocolCloseRequest.pipe(
  filter((protocol: ChildProcess) => !protocol.killed)
)

// Logging

debugObservables(
  [
    formatObservable(protocolLaunchProcess, formatChildProcess),
    "protocolLaunchProcess",
  ],
  [protocolLaunchSuccess, "protocolLaunchSuccess"],
  [protocolLoading, "protocolLaunchLoading"],
  [
    formatObservable(protocolCloseRequest, formatChildProcess),
    "protocolCloseRequest",
  ]
)

errorObservables([protocolLaunchFailure, "protocolLaunchFailure"])
