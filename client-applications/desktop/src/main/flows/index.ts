import { merge, of, fromEvent } from "rxjs"
import { mergeMap, map, zip } from "rxjs/operators"
import { EventEmitter } from "events"

import loginFlow from "@app/main/flows/login"
import containerFlow from "@app/main/flows/container"
import protocolLaunchFlow from "@app/main/flows/launch"
import protocolCloseFlow from "@app/main/flows/close"

import { fromTrigger } from "@app/utils/flows"

// persistFlow(fromTrigger("updateNotAvailable"))
loginFlow(fromTrigger("loginAction"))
// signupFlow(fromTrigger("signupAction"))

containerFlow(
  merge(
    fromTrigger("persistFlowSuccess"),
    fromTrigger("loginFlowSuccess"),
    fromTrigger("signupFlowSuccess")
  )
)

protocolLaunchFlow(
  merge(
    fromTrigger("persistFlowSuccess"),
    fromTrigger("loginFlowSuccess"),
    fromTrigger("signupFlowSuccess")
  )
)

protocolCloseFlow(
  fromTrigger("protocolLaunchFlowSuccess").pipe(
    mergeMap(({ protocol }) =>
      zip(of(protocol), fromEvent(protocol as EventEmitter, "close"))
    ),
    map(([protocol]) => {
      protocol
    })
  )
)
