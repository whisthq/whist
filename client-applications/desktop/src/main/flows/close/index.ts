// This file is home to observables that manage launching the protocol process
// on the user's machine. They are distinct from the observables that
// communicate with the webserver to manage container creation.
//
// Many of these observables emit the protocol ChildProcess object, which
// carries important data about the state of the protocol process.

import { ChildProcess } from "child_process"
import { flow, fork, trigger as _trigger } from "@app/main/utils/flows"

export default flow(
  "protocolCloseFlow",
  (trigger) => {
    const close = fork<ChildProcess>(trigger, {
      success: (protocol) => !protocol.killed,
      failure: (protocol) => protocol.killed,
    })

    return {
      success: _trigger("protocolCloseFlowSuccess", close.success),
      failure: _trigger("protocolCloseFlowFailure", close.failure),
    }
  },
)
