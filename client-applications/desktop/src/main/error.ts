import { identity } from "lodash"
import { ipcState, appReady } from "@app/main/events"
import { concat, race, of, combineLatest } from "rxjs"
import { pluck, filter, skip } from "rxjs/operators"
import { loginFailure } from "@app/main/login"
import { signupFailure } from "@app/main/signup"
import {
    createAuthErrorWindow,
    createContainerErrorWindow,
    createProtocolErrorWindow,
} from "@app/utils/windows"
import {
    containerAssignFailure,
    containerCreateFailure,
} from "@app/main/container"
import { protocolLaunchFailure } from "@app/main/protocol"

export const errorRelaunchRequest = ipcState.pipe(
    pluck("errorRelaunchRequest"),
    filter(identity)
)

export const errorWindowRequest = concat(
    appReady,
    race(
        combineLatest(loginFailure, of(createAuthErrorWindow)),
        combineLatest(signupFailure, of(createAuthErrorWindow)),
        combineLatest(containerCreateFailure, of(createContainerErrorWindow)),
        combineLatest(containerAssignFailure, of(createContainerErrorWindow)),
        combineLatest(protocolLaunchFailure, of(createProtocolErrorWindow))
    )
).pipe(skip(1)) // skip the appReady emit
