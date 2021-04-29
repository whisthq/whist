import { fromEventIPC } from "@app/main/events/ipc"
import { fromEventPersist } from "@app/main/events/persist"
import { loginSuccess } from "@app/main/observables/login"
import { signupRequest, signupSuccess } from "@app/main/observables/signup"
import { debugObservables } from "@app/utils/logging"
import { merge, from } from "rxjs"
import { identity } from "lodash"
import {
  map,
  sample,
  switchMap,
  withLatestFrom,
  share,
  filter,
} from "rxjs/operators"
import {
  emailLoginConfigToken,
  emailLoginAccessToken,
  emailLoginRefreshToken,
} from "@app/utils/login"
import {
  emailSignupAccessToken,
  emailSignupRefreshToken,
} from "@app/utils/signup"
import { formatObservable, formatTokens } from "@app/utils/formatters"

export const userEmail = merge(
  fromEventPersist("userEmail"),
  fromEventIPC("loginRequest", "email").pipe(sample(loginSuccess)),
  fromEventIPC("signupRequest", "email").pipe(sample(signupSuccess))
).pipe(filter(identity), share())

export const userConfigToken = merge(
  fromEventPersist("userConfigToken"),
  fromEventIPC("loginRequest", "password").pipe(
    sample(loginSuccess),
    withLatestFrom(loginSuccess),
    switchMap(([pw, res]) => from(emailLoginConfigToken(res, pw)))
  ),
  signupRequest.pipe(map(([_email, _password, token]) => token))
).pipe(filter(identity), share())

export const userAccessToken = merge(
  fromEventPersist("userAccessToken"),
  loginSuccess.pipe(map(emailLoginAccessToken)),
  signupSuccess.pipe(map(emailSignupAccessToken))
).pipe(filter(identity), share())

export const userRefreshToken = merge(
  fromEventPersist("userRefeshToken"),
  loginSuccess.pipe(map(emailLoginRefreshToken)),
  signupSuccess.pipe(map(emailSignupRefreshToken))
).pipe(filter(identity), share())

// Logging
debugObservables(
  [userEmail, "userEmail"],
  [formatObservable(userConfigToken, formatTokens), "userConfigToken"],
  [formatObservable(userAccessToken, formatTokens), "userAccessToken"],
  [formatObservable(userRefreshToken, formatTokens), "userRefreshToken"]
)
