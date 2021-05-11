// This file is home to observables that manage launching the protocol process
// on the user's machine. They are distinct from the observables that
// communicate with the webserver to manage container creation.
//
// Many of these observables emit the protocol ChildProcess object, which
// carries important data about the state of the protocol process.

import { zip, of, fromEvent, combineLatest } from "rxjs"
import { map, mergeMap } from "rxjs/operators"
import { EventEmitter } from "events"
import { ChildProcess } from "child_process"
import { protocolLaunch } from "@app/utils/protocol"
import {
  userEmail,
  userAccessToken,
  userConfigToken,
} from "@app/main/observables/user"
import { eventUpdateNotAvailable } from "@app/main/events/autoupdate"
import { gates, Flow } from "@app/utils/gates"

const protocolLaunchGates: Flow = (name, trigger) =>
  gates(name, trigger.pipe(map(() => protocolLaunch())), {
    success: () => true,
  })

const protocolCloseGates: Flow<ChildProcess> = (name, trigger) =>
  gates(name, trigger, {
    success: (protocol) => !protocol.killed,
    failure: (protocol) => protocol.killed,
  })

export const { success: protocolLaunchSuccess } = protocolLaunchGates(
  "protocolLaunch",
  combineLatest([
    zip(userEmail, userAccessToken, userConfigToken),
    eventUpdateNotAvailable,
  ])
)

export const {
  success: protocolCloseSuccess,
  failure: protocolCloseFailure,
} = protocolCloseGates(
  "protocolClose",
  protocolLaunchSuccess.pipe(
    mergeMap((protocol) =>
      zip(of(protocol), fromEvent(protocol as EventEmitter, "close"))
    ),
    map(([protocol]) => protocol)
  )
)
