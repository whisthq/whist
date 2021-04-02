import { ipcState } from "@app/main/events"
import { from, merge, of } from "rxjs"
import { emailLogin, emailLoginValid, emailLoginError } from "@app/utils/api"
import { pluck, mapTo, filter, map, share, exhaustMap } from "rxjs/operators"

export const loginRequest = ipcState.pipe(
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
