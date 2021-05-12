import { merge, of, fromEvent } from "rxjs"
import { mergeMap, map, zip } from "rxjs/operators"
import { EventEmitter } from "events"

import protocolLaunchFlow from "@app/main/flows/launch"
import protocolCloseFlow from "@app/main/flows/close"
import containerFlow from "@app/main/flows/container"
import loginFlow from "@app/main/flows/login"
import signupFlow from "@app/main/flows/signup"
import persistFlow from "@app/main/flows/persist"

import { fromTrigger } from "@app/main/utils/flows"

persistFlow(fromTrigger("autoupdateNotAvailable"))
loginFlow(fromTrigger("loginAction"))
signupFlow(fromTrigger("signupAction"))

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
