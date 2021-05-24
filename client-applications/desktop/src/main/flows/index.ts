import { merge, zip, fromEvent, of } from "rxjs"
import { map, mergeMap, take } from "rxjs/operators"
import { EventEmitter } from "events"
import { ChildProcess } from "child_process"

import loginFlow from "@app/main/flows/login"
import signupFlow from "@app/main/flows/signup"
import containerFlow from "@app/main/flows/container"
import protocolLaunchFlow from "@app/main/flows/launch"
import protocolCloseFlow from "@app/main/flows/close"
import autoUpdateFlow from "@app/main/flows/autoupdate"
import { fromTrigger } from "@app/utils/flows"
import { fromSignal } from "@app/utils/observables"

// Autoupdate flow
autoUpdateFlow(fromTrigger("updateAvailable"))

// Auth flows
loginFlow(fromTrigger("loginAction"))
signupFlow(fromTrigger("signupAction"))

// Observable that fires when Fractal is ready to be launched
const launchTrigger = merge(
  fromSignal(fromTrigger("persisted"), fromTrigger("updateNotAvailable")),
  fromTrigger("loginFlowSuccess"),
  fromTrigger("signupFlowSuccess")
).pipe(take(1))

// Container creation flow
containerFlow(launchTrigger)

// Protocol launch flow
protocolLaunchFlow(launchTrigger)

// Protocol close flow
protocolCloseFlow(
  fromTrigger("protocolLaunchFlowSuccess").pipe(
    mergeMap((protocol: ChildProcess) =>
      zip(of(protocol), fromEvent(protocol as EventEmitter, "close"))
    ),
    map(([protocol]) => protocol)
  )
)
