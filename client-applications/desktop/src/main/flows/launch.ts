// This file is home to observables that manage launching the protocol process
// on the user's machine. They are distinct from the observables that
// communicate with the webserver to manage container creation.
//
// Many of these observables emit the protocol ChildProcess object, which
// carries important data about the state of the protocol process.

import { map, take } from "rxjs/operators"
import { ChildProcess } from "child_process"

import { protocolLaunch } from "@app/utils/protocol"
import { flow, fork, createTrigger } from "@app/utils/flows"

export default flow("protocolLaunchFlow", (trigger) => {
  const launch = fork<ChildProcess>(trigger.pipe(map(() => protocolLaunch())), {
    success: () => true,
  })

  return {
    success: createTrigger("protocolLaunchFlowSuccess", launch.success),
  }
})
