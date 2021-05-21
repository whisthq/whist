import { merge, zip, fromEvent, of, combineLatest } from "rxjs"
import { map, mergeMap, take, tap, sample, switchMapTo } from "rxjs/operators"
import { EventEmitter } from "events"
import { ChildProcess } from "child_process"

import loginFlow from "@app/main/flows/login"
import signupFlow from "@app/main/flows/signup"
import containerFlow from "@app/main/flows/container"
import protocolLaunchFlow from "@app/main/flows/launch"
import protocolCloseFlow from "@app/main/flows/close"

import { fromTrigger } from "@app/utils/flows"

// Auth flows
loginFlow(fromTrigger("loginAction"))
signupFlow(fromTrigger("signupAction"))

fromTrigger("updateNotAvailable").subscribe(() =>
  console.log("UPDATE SHOULD PRINT HERE")
)

const launchTrigger = merge(
  combineLatest([
    fromTrigger("persisted"),
    fromTrigger("updateNotAvailable"),
    // ^^^^^^ protocol won't launch because this isn't firing
  ]).pipe(map(([a]) => a)),
  fromTrigger("loginFlowSuccess"),
  fromTrigger("signupFlowSuccess")
).pipe(take(1))

// Container flow
containerFlow(launchTrigger)

// Protocol flow
protocolLaunchFlow(launchTrigger)

protocolCloseFlow(
  fromTrigger("protocolLaunchFlowSuccess").pipe(
    mergeMap((protocol: ChildProcess) =>
      zip(of(protocol), fromEvent(protocol as EventEmitter, "close"))
    ),
    map(([protocol]) => protocol)
  )
)
