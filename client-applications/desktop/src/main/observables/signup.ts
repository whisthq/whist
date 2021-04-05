// This file is home to the observables that manage the signup page in the
// renderer thread. They listen to IPC events what contain signupRequest
// information, and they communicate with the server to authenticate the user.
//
// It's important to note that information received over IPC, like user email,
// or received as a response from the webserver, like an access token, are not
// subscribed directly by observables like userEmail and userConfigToken. All
// received data goes through the full "persistence cycle" first, where it is
// saved to local storage. userEmail and userConfigToken observables then
// "listen" to local storage, and update their values based on local
// storage changes.

import { fromEventIPC } from "@app/main/events/ipc"
import { from, merge, of, race } from "rxjs"
import { loadingFrom } from "@app/utils/observables"
import { emailSignup, emailSignupValid, emailSignupError } from "@app/utils/api"
import { createConfigToken } from "@app/utils/crypto"
import {
    mapTo,
    filter,
    map,
    share,
    exhaustMap,
    switchMap,
    startWith,
} from "rxjs/operators"

export const signupRequest = fromEventIPC("signupRequest").pipe(
    filter((req) => (req?.email && req?.password ? true : false)),
    switchMap((req) => from(createConfigToken().then((token) => [req, token]))),
    map(([req, token]) => [req?.email, req?.password, token]),
    share()
)

export const signupProcess = signupRequest.pipe(
    map(([email, password, token]) => emailSignup(email, password, token)),
    exhaustMap((req) => from(req)),
    share()
)

export const signupWarning = signupProcess.pipe(
    filter((res) => !emailSignupError(res)),
    filter((res) => !emailSignupValid(res))
)

export const signupSuccess = signupProcess.pipe(
    filter((res) => emailSignupValid(res))
)

export const signupFailure = signupProcess.pipe(
    filter((res) => emailSignupError(res))
)

export const signupLoading = loadingFrom(
    signupRequest,
    signupSuccess,
    signupFailure
)
