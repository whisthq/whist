import { eventIPC } from "@app/main/events"
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
