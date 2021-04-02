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

import { eventIPC } from "@app/main/events/events"
import { from, merge, of } from "rxjs"
import { emailSignup, emailSignupValid, emailSignupError } from "@app/utils/api"
import { pluck, mapTo, filter, map, share, exhaustMap } from "rxjs/operators"

export const signupRequest = eventIPC.pipe(
    // withLatestFrom()
    pluck("signupRequest"),
    map((req) => req as { email?: string; password?: string }),
    filter((req) => (req?.email && req?.password ? true : false)),
    map((req) => emailSignup(req.email!, req.password!)),
    share()
)

export const signupLoading = signupRequest.pipe(
    exhaustMap((req) => merge(of(true), from(req).pipe(mapTo(false))))
)

export const signupWarning = signupRequest.pipe(
    exhaustMap((req) => from(req)),
    filter((res) => !emailSignupError(res)),
    filter((res) => !emailSignupValid(res))
)

export const signupSuccess = signupRequest.pipe(
    exhaustMap((req) => from(req)),
    filter((res) => emailSignupValid(res)),
    share()
)

export const signupFailure = signupRequest.pipe(
    exhaustMap((req) => from(req)),
    filter((res) => emailSignupError(res))
)
