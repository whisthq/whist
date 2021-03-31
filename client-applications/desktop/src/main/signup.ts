import { ipcState } from "@app/main/events"
import { from, merge, of } from "rxjs"
import { WarningSignupInvalid } from "@app/utils/constants"
import { emailSignup, emailSignupValid, emailSignupError } from "@app/utils/api"
import { pluck, mapTo, filter, map, share, exhaustMap } from "rxjs/operators"

export const SignupRequest = ipcState.pipe(
    pluck("signupRequest"),
    map((req) => req as { email?: string; password?: string }),
    filter((req) => (req?.email && req?.password ? true : false)),
    map((req) => emailSignup(req.email!, req.password!)),
    share()
)

export const SignupLoading = SignupRequest.pipe(
    exhaustMap((req) => merge(of(true), from(req).pipe(mapTo(false))))
)

export const SignupWarning = SignupRequest.pipe(
    exhaustMap((req) => from(req).pipe(mapTo(WarningSignupInvalid))),
    filter((res) => !emailSignupValid(res))
)

export const SignupSuccess = SignupRequest.pipe(
    exhaustMap((req) => from(req)),
    filter((res) => emailSignupValid(res))
)

export const SignupFailure = SignupRequest.pipe(
    exhaustMap((req) => from(req)),
    filter((res) => emailSignupError(res))
)
