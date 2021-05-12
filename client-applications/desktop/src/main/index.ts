import { app } from "electron"
import { flow, withEffects } from "@app/utils/flows"
import { merge, fromEvent, combineLatest } from "rxjs"
import { mapTo } from "rxjs/operators"
import { ChildProcess } from "child_process"

import protocolLaunchFlow from "@app/main/flows/protocol"
import containerFlow from "@app/main/flows/container"
import loginFlow from "@app/main/flows/login"
import signupFlow from "@app/main/flows/signup"
import persistFlow from "@app/main/flows/persist"

import { fromTrigger } from "@app/utils/flows"

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

