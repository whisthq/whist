// This file is home to observables that manage launching the protocol process
// on the user's machine. They are distinct from the observables that
// communicate with the webserver to manage container creation.
//
// Many of these observables emit the protocol ChildProcess object, which
// carries important data about the state of the protocol process.

import { Observable } from "rxjs"
import { map } from "rxjs/operators"
import { protocolLaunch } from "@app/utils/protocol"
import { flow, fork } from "@app/utils/flows"

export const protocolLaunchFlow = flow(
  "protocolLaunchFlow",
  (_name, trigger) => {
    const launch = fork(trigger.pipe(map(() => protocolLaunch())), {
      success: () => true,
    })

    return {
      success: launch.success,
    }
  }
)

export const protocolCloseFlow = flow(
  "protocolCloseFlow",
  (_name, trigger: Observable<any>) => {
    const close = fork(trigger, {
      success: (protocol) => !protocol.killed,
      failure: (protocol) => protocol.killed,
    })

    return {
      success: close.success,
      failure: close.failure,
    }
  }
)
