import { merge, fromEvent } from "rxjs"
import { map, mergeMap, take } from "rxjs/operators"
import { EventEmitter } from "events"
import { ChildProcess } from "child_process"

import authFlow from "@app/main/flows/auth"
import mandelboxFlow from "@app/main/flows/mandelbox"
import protocolLaunchFlow from "@app/main/flows/launch"
import protocolCloseFlow from "@app/main/flows/close"
import autoUpdateFlow from "@app/main/flows/autoupdate"
import { fromTrigger } from "@app/utils/flows"
import { fromSignal } from "@app/utils/observables"
import { getRegionFromArgv } from "@app/utils/region"

// Autoupdate flow
autoUpdateFlow(fromTrigger("updateAvailable"))

// Auth flow
authFlow(
  fromSignal(
    merge(
      fromSignal(fromTrigger("authInfo"), fromTrigger("notPersisted")),
      fromTrigger("persisted")
    ),
    fromTrigger("updateNotAvailable")
  )
)

// Observable that fires when Fractal is ready to be launched
const launchTrigger = fromTrigger("authFlowSuccess").pipe(
  map((x: object) => ({
    ...x, // { sub, accessToken, configToken }
    region: getRegionFromArgv(process.argv), // AWS region, if admins want to control the region
  })),
  take(1)
)

// Mandelbox creation flow
mandelboxFlow(launchTrigger)

// Protocol launch flow
protocolLaunchFlow(launchTrigger)

// Protocol close flow
const close = protocolCloseFlow(
  fromTrigger("protocolLaunchFlowSuccess").pipe(
    mergeMap((protocol: ChildProcess) =>
      fromEvent(protocol as EventEmitter, "close").pipe(map(() => protocol))
    )
  )
)

// Subscribe so that the protocolCloseFlow actually emits
// (if an observable has no subscribers it won't emit)
close.success.subscribe()
close.failure.subscribe()
