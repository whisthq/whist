// This file is home to observables related to error handling in the application.
// These should subscribe to "Failure" observables, and emit useful data
// for error-related side-effects defined in effects.ts.

import { identity } from "lodash"
import { eventIPC } from "@app/main/events/ipc"
import { eventAppReady } from "@app/main/events/app"
import { merge } from "rxjs"
import { pluck, filter, map, mapTo, withLatestFrom } from "rxjs/operators"
import { loginFailure } from "@app/main/observables/login"
import { signupFailure } from "@app/main/observables/signup"
import {
    createAuthErrorWindow,
    createContainerErrorWindowNoAccess,
    createContainerErrorWindowUnauthorized,
    createContainerErrorWindowInternal,
    assignContainerErrorWindow,
    createProtocolErrorWindow,
} from "@app/utils/windows"
import {
    containerAssignFailure,
    containerCreateFailureNoAccess,
    containerCreateFailureUnauthorized,
    containerCreateFailureInternal,
} from "@app/main/observables/container"
import { protocolLaunchFailure } from "@app/main/observables/protocol"

export const errorRelaunchRequest = eventIPC.pipe(
    pluck("errorRelaunchRequest"),
    filter(identity)
)

export const errorWindowRequest = merge(
    loginFailure.pipe(mapTo(createAuthErrorWindow)),
    signupFailure.pipe(mapTo(createAuthErrorWindow)),
    containerCreateFailureNoAccess.pipe(
        mapTo(createContainerErrorWindowNoAccess)
    ),
    containerCreateFailureUnauthorized.pipe(
        mapTo(createContainerErrorWindowUnauthorized)
    ),
    containerCreateFailureInternal.pipe(
        mapTo(createContainerErrorWindowInternal)
    ),
    containerAssignFailure.pipe(mapTo(assignContainerErrorWindow)),
    protocolLaunchFailure.pipe(mapTo(createProtocolErrorWindow))
).pipe(
    withLatestFrom(eventAppReady),
    map(([f, _]) => f)
)
