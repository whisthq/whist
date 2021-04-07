import { fromEventIPC } from "@app/main/events/ipc"
import { loginSuccess } from "@app/main/observables/login"
import { signupRequest, signupSuccess } from "@app/main/observables/signup"
import { merge, from } from "rxjs"
import { map, sample, switchMap, withLatestFrom } from "rxjs/operators"
import {
    emailLoginConfigToken,
    emailLoginAccessToken,
    emailLoginRefreshToken,
    emailSignupAccessToken,
    emailSignupRefreshToken,
} from "@app/utils/api"

export const userEmail = merge(
    fromEventIPC("loginRequest", "email").pipe(sample(loginSuccess)),
    fromEventIPC("signupRequest", "email").pipe(sample(signupSuccess))
)

export const userConfigToken = merge(
    fromEventIPC("loginRequest", "password").pipe(
        sample(loginSuccess),
        withLatestFrom(loginSuccess),
        switchMap(([pw, res]) => from(emailLoginConfigToken(res, pw)))
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
