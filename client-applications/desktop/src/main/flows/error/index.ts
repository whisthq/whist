// This file is home to observables related to error handling in the application.
// These should subscribe to "Failure" observables, and emit useful data
// for error-related side-effects defined in effects.ts.

import { identity } from "lodash"
import { eventIPC } from "@app/main/events/ipc"
import { eventAppReady } from "@app/main/events/app"
import { merge } from "rxjs"
import { pluck, filter, map, mapTo, withLatestFrom } from "rxjs/operators"
import { loginFailure } from "@app/main/flows/auth/flows/login"
import { signupFailure } from "@app/main/flows/auth/flows/signup"
import {
  createAuthErrorWindow,
  createContainerErrorWindowNoAccess,
  createContainerErrorWindowUnauthorized,
  createContainerErrorWindowInternal,
  assignContainerErrorWindow,
  createProtocolErrorWindow,
} from "@app/main/flows/error/utils"
import {
  containerCreateErrorNoAccess,
  containerCreateErrorUnauthorized,
} from "@app/main/flows/container/flows/create/utils"
import { warningObservables } from "@app/utils/logging"
import {
  containerPollingFailure,
  containerCreateFailure,
} from "@app/main/flows/container/flows/create"

export const errorRelaunchRequest = eventIPC.pipe(
  pluck("errorRelaunchRequest"),
  filter(identity)
)

export const errorWindowRequest = merge(
  loginFailure.pipe(mapTo(createAuthErrorWindow)),
  signupFailure.pipe(mapTo(createAuthErrorWindow)),
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
  containerPollingFailure.pipe(mapTo(assignContainerErrorWindow))
).pipe(
  withLatestFrom(eventAppReady),
  map(([f, _]) => f)
)

// Logging

warningObservables([errorRelaunchRequest, "errorRelaunchRequest"])
