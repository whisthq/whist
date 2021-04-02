import { app } from "electron"
import { mapValues, identity } from "lodash"
import { persist } from "@app/utils/persist"
import { ipcState, appReady } from "@app/main/events"
import { ipcBroadcast } from "@app/utils/state"
import { zip, merge, concat, race, combineLatest, of } from "rxjs"
import {
    startWith,
    tap,
    map,
    mapTo,
    skip,
    pluck,
    filter,
    switchMap,
} from "rxjs/operators"
import { protocolStreamInfo, protocolStreamKill } from "@app/utils/protocol"
import { createConfigToken } from "@app/utils/crypto"
import { WarningLoginInvalid } from "@app/utils/constants"
import {
    emailLoginConfigToken,
    emailLoginAccessToken,
    emailLoginRefreshToken,
    emailSignupConfigToken,
    emailSignupAccessToken,
    emailSignupRefreshToken,
} from "@app/utils/api"
import {
    getWindows,
    closeWindows,
    createAuthWindow,
    createAuthErrorWindow,
    createContainerErrorWindow,
    createProtocolErrorWindow,
} from "@app/utils/windows"
import { userEmail, userAccessToken, userConfigToken } from "@app/main/user"
import {
    loginRequest,
    loginLoading,
    loginWarning,
    loginFailure,
    loginSuccess,
} from "@app/main/login"
import {
    signupRequest,
    signupLoading,
    signupWarning,
    signupFailure,
    signupSuccess,
} from "@app/main/signup"

import {
    containerCreateRequest,
    containerCreateLoading,
    containerCreateSuccess,
    containerCreateFailure,
    containerAssignRequest,
    containerAssignLoading,
    containerAssignPolling,
    containerAssignSuccess,
    containerAssignFailure,
} from "@app/main/container"
import {
    protocolLaunchRequest,
    protocolLaunchLoading,
    protocolLaunchSuccess,
    protocolLaunchFailure,
    protocolCloseRequest,
    protocolCloseFailure,
} from "@app/main/protocol"

import { errorRelaunchRequest, errorWindowRequest } from "@app/main/error"

// Persistence
// We create observables for each piece of state we want to persist.
// Each observable subscribes to the parts of the application that provide
// state updates.
//
// They are combined into a dictionary, which is persisted to local storage.
const persistUserEmail = ipcState.pipe(pluck("signupRequest", "email"))

const persistUserAccessToken = merge(
    loginSuccess.pipe(map(emailLoginAccessToken)),
    signupSuccess.pipe(map(emailSignupAccessToken))
)

const persistUserConfigToken = merge(
    ipcState.pipe(
        filter((req: any) => req.signupRequest),
        map(() => createConfigToken())
    ),
    zip(
        ipcState.pipe(pluck("signupRequest", "password"), filter(identity))
    ).pipe(switchMap((args) => emailLoginConfigToken(...args)))
)

const persistState = combineLatest(
    mapValues(
        {
            userEmail: persistUserEmail,
            userAccessToken: persistUserAccessToken,
            userConfigToken: persistUserConfigToken,
        },
        (o: any): any => o.pipe(startWith(undefined))
    )
)

persistState.subscribe((state) => persist(state))

// Window opening
// appReady only fires once, at the launch of the application.
appReady.subscribe(() => {
    createAuthWindow((win: any) => win.show())
})

// Window closing
// If we have an access token and an email, close the existing windows.
zip(userEmail, userAccessToken).subscribe(() => {
    closeWindows()
})

// Application closing
errorRelaunchRequest.subscribe((req) => {
    if (req) {
        app.relaunch()
        app.exit()
    }
})

// Error windows
errorWindowRequest.subscribe(([_failure, createWindow]) => {
    closeWindows(), createWindow()
})

// Streaming information to protocol
// Stream the ip, secret_key, and ports info to the protocol when we
// when we receive a successful container status response.
zip(
    protocolLaunchRequest,
    protocolLaunchSuccess
).subscribe(([protocol, info]) => protocolStreamInfo(protocol, info))

// Protocol closing
// If we have an error, close the protocol.
zip(
    protocolLaunchRequest,
    protocolLaunchFailure
).subscribe(([protocol, _error]) => protocolStreamKill(protocol))

// Broadcast state to all renderer windows.
combineLatest(
    ipcState,
    loginLoading.pipe(startWith(false)),
    loginWarning.pipe(mapTo(WarningLoginInvalid), startWith(null))
).subscribe(([state, loginLoading, loginWarning]) =>
    ipcBroadcast({ ...state, loginLoading, loginWarning }, getWindows())
)
