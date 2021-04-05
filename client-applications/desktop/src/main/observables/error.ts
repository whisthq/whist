// This file is home to observables related to error handling in the application.
// These should subscribe to "Failure" observables, and emit useful data
// for error-related side-effects defined in effects.ts.

import { identity } from "lodash"
import { eventIPC } from "@app/main/events/ipc"
import { eventAppReady } from "@app/main/events/app"
import { concat, merge } from "rxjs"
import { pluck, filter, skip, share, mapTo } from "rxjs/operators"
import { loginFailure } from "@app/main/observables/login"
import { signupFailure } from "@app/main/observables/signup"
import {
    createAuthErrorWindow,
    createContainerErrorWindow,
    createProtocolErrorWindow,
} from "@app/utils/windows"
import {
    containerAssignFailure,
    containerCreateFailure,
} from "@app/main/observables/container"
import { protocolLaunchFailure } from "@app/main/observables/protocol"

export const errorRelaunchRequest = eventIPC.pipe(
    pluck("errorRelaunchRequest"),
    filter(identity)
)

export const errorWindowRequest = concat(
    eventAppReady,
    merge(
        loginFailure.pipe(mapTo(createAuthErrorWindow)),
        signupFailure.pipe(mapTo(createAuthErrorWindow)),
        containerCreateFailure.pipe(mapTo(createContainerErrorWindow)),
        containerAssignFailure.pipe(mapTo(createContainerErrorWindow)),
        protocolLaunchFailure.pipe(mapTo(createProtocolErrorWindow))
    )
).pipe(skip(1), share()) // skip the appReady emit
