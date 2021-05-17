import { merge } from "rxjs"
import { skipUntil } from "rxjs/operators"
import { EventEmitter } from "events"
import { ChildProcess } from "child_process"

import loginFlow from "@app/main/flows/login"
import signupFlow from "@app/main/flows/signup"
import containerFlow from "@app/main/flows/container"
import protocolLaunchFlow from "@app/main/flows/launch"
import protocolCloseFlow from "@app/main/flows/close"

import { fromTrigger } from "@app/utils/flows"


loginFlow(fromTrigger("loginAction"))
signupFlow(fromTrigger("signupAction"))

containerFlow(
  merge(
    fromTrigger("persisted"),
    fromTrigger("loginFlowSuccess"),
    fromTrigger("signupFlowSuccess")
  )
)

protocolLaunchFlow(
  merge(
    fromTrigger("persisted"),
    fromTrigger("loginFlowSuccess"),
    fromTrigger("signupFlowSuccess")
  )
)

// protocolCloseFlow(
//   fromTrigger("protocolLaunchFlowSuccess").pipe(
//     mergeMap((protocol: ChildProcess) =>
//       zip(of(protocol), fromEvent(protocol as EventEmitter, "close"))
//     ),
//     map(([protocol]) => {
//       protocol
//     })
//   )
// )
