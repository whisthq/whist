// This file is home to observables that manage launching the protocol process
// on the user's machine. They are distinct from the observables that
// communicate with the webserver to manage container creation.
//
// Many of these observables emit the protocol ChildProcess object, which
// carries important data about the state of the protocol process.

import { map } from "rxjs/operators"
import { protocolLaunch } from "@app/utils/protocol"
import { gates, Flow } from "@app/utils/gates"

export const protocolLaunchFlow: Flow = (name, trigger) => {
  const next = `${name}.protocolLaunchFlow`

  const launch = gates(next, trigger.pipe(map(() => protocolLaunch())), {
    success: () => true,
  })

  return {
    success: launch.success,
  }
}

export const protocolCloseFlow: Flow = (name, trigger) => {
  const next = `${name}.protocolCloseFlow`

  const close = gates(next, trigger, {
    success: (protocol) => !protocol.killed,
    failure: (protocol) => protocol.killed,
  })

  return {
    success: close.success,
    failure: close.failure,
  }
}
