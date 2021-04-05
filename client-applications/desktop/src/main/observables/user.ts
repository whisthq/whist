import { fromEventIPC } from "@app/main/events/ipc"
import { loginSuccess } from "@app/main/observables/login"
import { signupRequest, signupSuccess } from "@app/main/observables/signup"
import { merge, from } from "rxjs"
import { map, switchMap, withLatestFrom, sample } from "rxjs/operators"
import {
    emailLoginConfigToken,
    emailLoginAccessToken,
    emailLoginRefreshToken,
    emailSignupAccessToken,
    emailSignupRefreshToken,
} from "@app/utils/api"

export const userEmail = merge(
    loginSuccess.pipe(sample(fromEventIPC("loginRequest", "email"))),
    signupSuccess.pipe(sample(fromEventIPC("signupRequest", "email")))
)

export const userConfigToken = merge(
    loginSuccess.pipe(
        withLatestFrom(fromEventIPC("loginRequest", "password")),
        switchMap((args) => from(emailLoginConfigToken(...args)))
    ),
    signupRequest.pipe(map(([_email, _password, token]) => token))
)

export const userAccessToken = merge(
    loginSuccess.pipe(map(emailLoginAccessToken)),
    signupSuccess.pipe(map(emailSignupAccessToken))
)

export const userRefreshToken = merge(
    loginSuccess.pipe(map(emailLoginRefreshToken)),
    signupSuccess.pipe(map(emailSignupRefreshToken))
)
