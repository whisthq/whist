import { merge, zip, fromEvent, of } from "rxjs"
import { map, mergeMap, take } from "rxjs/operators"
import { EventEmitter } from "events"

import loginFlow from "@app/main/flows/login"
import signupFlow from "@app/main/flows/signup"
import containerFlow from "@app/main/flows/container"
import protocolLaunchFlow from "@app/main/flows/launch"
import protocolCloseFlow from "@app/main/flows/close"

import { fromTrigger } from "@app/utils/flows"

// Auth flows
loginFlow(fromTrigger("loginAction"))
signupFlow(fromTrigger("signupAction"))

// Container flow
containerFlow(
  merge(
    fromTrigger("persisted"),
    fromTrigger("loginFlowSuccess"),
    fromTrigger("signupFlowSuccess")
  ).pipe(take(1))
)

protocolLaunchFlow(
  merge(
    fromTrigger("persisted"),
    fromTrigger("loginFlowSuccess"),
    fromTrigger("signupFlowSuccess")
  ).pipe(take(1))
)

protocolCloseFlow(
  fromTrigger("protocolLaunchFlowSuccess").pipe(
    mergeMap((protocol) =>
      zip(of(protocol), fromEvent(protocol as EventEmitter, "close"))
    ),
    map(([protocol]) => {
      protocol
    })
  )
)
