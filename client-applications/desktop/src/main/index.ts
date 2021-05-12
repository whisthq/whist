import { app } from "electron"
import { flow, withEffects } from "@app/utils/flows"
import { merge, fromEvent, zip, combineLatest } from "rxjs"
import { mapTo } from "rxjs/operators"
import { ChildProcess } from "child_process"

import protocolLaunchFlow from "@app/main/flows/protocol"
import containerFlow from "@app/main/flows/container"
import authFlow from "@app/main/flows/auth"
import errorFlow from "@app/main/flows/error"
import { protocolStreamInfo } from "@app/main/flows/protocol/flows/launch/utils"

// Flow to launch Google Chrome
const chromeFlow = flow("chromeFlow", (name, trigger) => {
  // Trigger container creation and protocol launch asynchronously
  const container = containerFlow(name, trigger)
  const protocol = protocolLaunchFlow(name, trigger)

  return {
    success: combineLatest({
      protocol: protocol.success,
      container: container.success,
    }),
    failure: merge(container.failure, protocol.failure),
  }
})

// Composes all other flows together. This is the entry point of the application.
const mainFlow = flow("mainFlow", (name, trigger) => {
  // First, authenticate the user
  const auth = authFlow(name, trigger)
  
  // Then, launch Google Chrome
  const chrome = withEffects(
    chromeFlow(name, auth.success),
    (args: { protocol: ChildProcess; info: any }) => {
      protocolStreamInfo(args.protocol, args.info)
    }
  )

  return {
    success: zip(auth.success, chrome.success).pipe(mapTo({})),
    failure: merge(auth.failure, chrome.failure),
  }
})

// Kick off the main flow to run the app.
errorFlow("app", mainFlow("app", fromEvent(app, "ready")).failure)
