// This is the application's main entry point. No code should live here, this
// file's only purpose is to "initialize" other files in the application by
// importing them.
//
// This file should import every other file in the top-level of the "main
// folder. While many of those files import each other and would run anyways,
// we import everything here first for the sake of being explicit.
//
// If you've created a new file in "main/" and you're not seeing it run, you
// probably haven't imported it here.

// import "@app/main/observables"
// import "@app/main/events"
// import "@app/main/effects"

import { app } from "electron"
import { flow } from "@app/utils/flows"
import { merge, fromEvent, zip } from "rxjs"
import { ChildProcess } from "child_process"

import { protocolLaunchFlow } from "@app/main/flows/protocol"
import containerFlow from "@app/main/flows/container"
import authFlow from "@app/main/flows/auth"
import { protocolStreamInfo } from "@app/main/flows/protocol/protocol"

// Composes all other flows together, and is the "entry" point of the
// application.
const mainFlow = flow("mainFlow", (name, trigger) => {
  // First, authenticate the user
  const auth = authFlow(name, trigger)

  // Once the user is authenticated, trigger container creation and protocol launch asynchronously
  const container = containerFlow(name, auth.success)
  const protocol = protocolLaunchFlow(name, auth.success)

  // TODO: Move side effect out of the flow
  zip(protocol.success, container.success).subscribe(
    ([protocol, info]: [ChildProcess, any]) => {
      protocolStreamInfo(protocol, info)
    }
  )

  return {
    success: zip(auth.success, container.success, protocol.success),
    failure: merge(auth.failure, container.failure, protocol.failure),
  }
})

// Kick off the main flow to run the app.
mainFlow("app", fromEvent(app, "ready"))
