import { fromEventPersist } from "@app/main/events/persist"
import { loginSuccess } from "@app/main/observables/login"
import {
  signupConfigSuccess,
  signupSuccess,
} from "@app/main/observables/signup"
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
  pluck,
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
import { loginAction, signupAction } from "@app/main/events/actions"
import { formatObservable, formatTokens } from "@app/utils/formatters"

export const userEmail = merge(
  fromEventPersist("userEmail"),
  loginAction.pipe(pluck("email"), sample(loginSuccess)),
  signupAction.pipe(pluck("email"), sample(signupSuccess))
).pipe(filter(identity), share())

export const userConfigToken = merge(
  fromEventPersist("userConfigToken"),
  loginAction.pipe(
    pluck("password"),
    sample(loginSuccess),
    withLatestFrom(loginSuccess),
    switchMap(([pw, res]) => from(emailLoginConfigToken(res, pw)))
  ),
  signupConfigSuccess
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
