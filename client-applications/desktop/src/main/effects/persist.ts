/**
 * Copyright Fractal Computers, Inc. 2021
 * @file app.ts
 * @brief This file contains subscriptions to Observables related to state persistence.
*/

import { mapValues, identity } from "lodash"
import { persist } from "@app/utils/persist"
import { eventIPC } from "@app/main/events/ipc"
import { ipcBroadcast } from "@app/utils/state"
import { zip, merge, combineLatest } from "rxjs"
import {
    startWith,
    map,
    mapTo,
    pluck,
    filter,
    switchMap,
} from "rxjs/operators"
import { createConfigToken } from "@app/utils/crypto"
import { WarningLoginInvalid } from "@app/utils/constants"
import {
    emailLoginConfigToken,
    emailLoginAccessToken,
    emailSignupAccessToken,
} from "@app/utils/api"
import {
    getWindows,
} from "@app/utils/windows"
import {
    loginLoading,
    loginWarning,
    loginSuccess,
} from "@app/main/observables/login"
import {
    signupSuccess,
} from "@app/main/observables/signup"


// Persistence
// We create observables for each piece of state we want to persist.
// Each observable subscribes to the parts of the application that provide
// state updates.
//
// They are combined into a dictionary, which is persisted to local storage.
const persistUserEmail = eventIPC.pipe(pluck("signupRequest", "email"))

const persistUserAccessToken = merge(
    loginSuccess.pipe(map(emailLoginAccessToken)),
    signupSuccess.pipe(map(emailSignupAccessToken))
)

const persistUserConfigToken = merge(
    eventIPC.pipe(
        filter((req: any) => req.signupRequest),
        map(() => createConfigToken())
    ),
    zip(
        eventIPC.pipe(pluck("signupRequest", "password"), filter(identity))
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

// Broadcast state to all renderer windows.
combineLatest(
    eventIPC,
    loginLoading.pipe(startWith(false)),
    loginWarning.pipe(mapTo(WarningLoginInvalid), startWith(null))
).subscribe(([state, loginLoading, loginWarning]) =>
    ipcBroadcast({ ...state, loginLoading, loginWarning }, getWindows())
)
