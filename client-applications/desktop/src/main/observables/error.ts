// This file is home to observables related to error handling in the application.
// These should subscribe to "Failure" observables, and emit useful data
// for error-related side-effects defined in effects.ts.

import { identity } from "lodash"
import { eventIPC, eventAppReady } from "@app/main/events/events"
import { concat, race, of, combineLatest } from "rxjs"
import { pluck, filter, skip } from "rxjs/operators"
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
    race(
        combineLatest(loginFailure, of(createAuthErrorWindow)),
        combineLatest(signupFailure, of(createAuthErrorWindow)),
        combineLatest(containerCreateFailure, of(createContainerErrorWindow)),
        combineLatest(containerAssignFailure, of(createContainerErrorWindow)),
        combineLatest(protocolLaunchFailure, of(createProtocolErrorWindow))
    )
).pipe(skip(1)) // skip the appReady emit
