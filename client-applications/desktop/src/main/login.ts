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

import { eventIPC } from "@app/main/events"
import { from, merge, of } from "rxjs"
import { emailLogin, emailLoginValid, emailLoginError } from "@app/utils/api"
import { pluck, mapTo, filter, map, share, exhaustMap } from "rxjs/operators"

export const loginRequest = eventIPC.pipe(
    pluck("loginRequest"),
    map((req) => req as { email?: string; password?: string }),
    filter((req) => (req?.email && req?.password ? true : false)),
    map((req) => emailLogin(req.email!, req.password!)),
    share()
)

export const loginLoading = loginRequest.pipe(
    exhaustMap((req) => merge(of(true), from(req).pipe(mapTo(false))))
)

export const loginWarning = loginRequest.pipe(
    exhaustMap((req) => from(req)),
    filter((res) => !emailLoginError(res)),
    filter((res) => !emailLoginValid(res))
)

export const loginSuccess = loginRequest.pipe(
    exhaustMap((req) => from(req)),
    filter((res) => emailLoginValid(res))
)

export const loginFailure = loginRequest.pipe(
    exhaustMap((req) => from(req)),
    filter((res) => emailLoginError(res))
)
