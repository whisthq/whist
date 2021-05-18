import { merge, zip, fromEvent, of } from "rxjs"
import { map, mergeMap, tap } from "rxjs/operators"
import { EventEmitter } from "events"
import { ChildProcess } from "child_process"

import loginFlow from "@app/main/flows/login"
import signupFlow from "@app/main/flows/signup"
import containerFlow from "@app/main/flows/container"
import protocolLaunchFlow from "@app/main/flows/launch"
import protocolCloseFlow from "@app/main/flows/close"

import { fromTrigger, initFlow } from "@app/utils/flows"

initFlow(fromTrigger("loginAction"), loginFlow)
initFlow(fromTrigger("signupAction"), signupFlow)

initFlow(
  merge(
    fromTrigger("persisted"),
    fromTrigger("loginFlowSuccess"),
    fromTrigger("signupFlowSuccess")
  ),
  containerFlow
)

initFlow(
  merge(
    fromTrigger("persisted"),
    fromTrigger("loginFlowSuccess"),
    fromTrigger("signupFlowSuccess")
  ),
  protocolLaunchFlow
)

initFlow(
  fromTrigger("protocolLaunchFlowSuccess").pipe(
    mergeMap((protocol) =>
      zip(of(protocol), fromEvent(protocol as ChildProcess, "close"))
    ),
    map(([protocol]) => {
      protocol
    })
  ),
  protocolCloseFlow
)
