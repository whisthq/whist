// This file is home to the observables that manage the login page in the
// renderer thread. They listen to IPC events what contain loginRequest
// information, and they communicate with the server to authenticate the user.
//
// It's important to note that information received over IPC, like user email,
// or received as a response from the webserver, like an access token, are not
// subscribed directly by observables like userEmail and userAccessToken. All
// received data goes through the full "persistence cycle" first, where it is
// saved to local storage. userEmail and userAccessToken observables then
// "listen" to local storage, and update their values based on local
// storage changes.

import { fromEventIPC } from "@app/main/events/ipc"
import { from } from "rxjs"
import { loadingFrom } from "@app/utils/observables"
import { emailLogin, emailLoginValid, emailLoginError } from "@app/utils/api"
import { filter, map, share, exhaustMap } from "rxjs/operators"

export const loginRequest = fromEventIPC("loginRequest").pipe(
    filter((req) => (req?.email && req?.password ? true : false)),
    map(({ email, password }) => [email, password]),
    share()
)

export const loginProcess = loginRequest.pipe(
    map(([email, password]) => emailLogin(email, password)),
    exhaustMap((req) => from(req)),
    share()
)

export const loginWarning = loginProcess.pipe(
    filter((res) => !emailLoginError(res)),
    filter((res) => !emailLoginValid(res))
)

export const loginSuccess = loginProcess.pipe(
    filter((res) => emailLoginValid(res))
)

export const loginFailure = loginProcess.pipe(
    filter((res) => emailLoginError(res))
)

export const loginLoading = loadingFrom(
    loginRequest,
    loginSuccess,
    loginFailure,
    loginWarning
)
