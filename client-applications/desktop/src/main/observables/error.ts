// This file is home to observables related to error handling in the application.
// These should subscribe to "Failure" observables, and emit useful data
// for error-related side-effects defined in effects.ts.

import { identity } from "lodash"
import { eventIPC } from "@app/main/events/ipc"
import { eventAppReady } from "@app/main/events/app"
import { merge } from "rxjs"
import { pluck, filter, map, mapTo, withLatestFrom } from "rxjs/operators"
import {
  createContainerErrorWindowNoAccess,
  createContainerErrorWindowUnauthorized,
  createContainerErrorWindowInternal,
  assignContainerErrorWindow,
  createProtocolErrorWindow,
} from "@app/utils/windows"
import {
  containerCreateErrorNoAccess,
  containerCreateErrorUnauthorized,
} from "@app/utils/container"
import { warningObservables } from "@app/utils/logging"
import {
  containerPollingFailure,
  containerCreateFailure,
} from "@app/main/observables/container"
import { protocolLaunchFailure } from "@app/main/observables/protocol"
import { userSub } from "@app/main/observables/user"

export const errorRelaunchRequest = eventIPC.pipe(
  pluck("errorRelaunchRequest"),
  filter(identity)
)

export const errorWindowRequest = merge(
  containerCreateFailure.pipe(
    map((response) => {
      if (containerCreateErrorNoAccess(response)) {
        return createContainerErrorWindowNoAccess
      }
      if (containerCreateErrorUnauthorized(response)) {
        return createContainerErrorWindowUnauthorized
      }
      return createContainerErrorWindowInternal
    })
  ),
  containerPollingFailure.pipe(mapTo(assignContainerErrorWindow)),
  protocolLaunchFailure.pipe(mapTo(createProtocolErrorWindow)),
  eventUpdateError.pipe(mapTo(createProtocolErrorWindow))
).pipe(
  withLatestFrom(eventAppReady),
  map(([f, _]) => f)
)

// Logging

warningObservables([errorRelaunchRequest, userSub, "errorRelaunchRequest"])
