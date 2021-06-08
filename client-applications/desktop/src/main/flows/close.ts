// This file is home to observables that manage launching the protocol process
// on the user's machine. They are distinct from the observables that
// communicate with the webserver to manage mandelbox creation.
//
// Many of these observables emit the protocol ChildProcess object, which
// carries important data about the state of the protocol process.

import { flow, createTrigger } from "@app/utils/flows"

export default flow("protocolCloseFlow", (trigger) => {
  return {
    success: createTrigger("protocolCloseFlowSuccess", trigger),
  }
})
