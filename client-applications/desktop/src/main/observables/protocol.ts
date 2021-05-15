// This file is home to observables that manage launching the protocol process
// on the user's machine. They are distinct from the observables that
// communicate with the webserver to manage container creation.
//
// Many of these observables emit the protocol ChildProcess object, which
// carries important data about the state of the protocol process.

import { map } from "rxjs/operators"
import { protocolLaunch } from "@app/utils/protocol"
import { ChildProcess } from "child_process"
import { flow, fork } from "@app/utils/flows"

export const protocolLaunchFlow = flow(
  "protocolLaunchFlow",
  (trigger) => {
    const launch = fork(trigger.pipe(map(() => protocolLaunch())), {
      success: () => true,
    })

    return {
      success: launch.success,
    }
  }
)

export const protocolCloseFlow = flow<ChildProcess>(
  "protocolCloseFlow",
  (trigger) => {
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
