// This file is home to observables of user data. This data should be sourced
// directly from persisted storage, which is the "beginning" of the reactive
// data flow in the application.
//
// It may be tempting to pull user data from other observables in the
// application, such as getting the userAccessToken directly from
// loginSuccess, but that can create a circular dependency.
//
// Instead, it's expected that data returned from observables like loginSuccess
// or signupSuccess will go through the "persistence cycle" and be first saved
// to local storage. The observables in this file then "listen" to local storage
// to receive the updated data.

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
