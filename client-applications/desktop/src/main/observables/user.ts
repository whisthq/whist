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
import { fromAction } from "@app/utils/actions"
import { RendererAction } from "@app/@types/actions"

export const userEmail = merge(
  fromEventPersist("userEmail"),
  fromAction(RendererAction.LOGIN, "email").pipe(sample(loginSuccess)),
  fromAction(RendererAction.SIGNUP, "email").pipe(sample(signupSuccess))
).pipe(filter(identity), share())

export const userConfigToken = merge(
  fromEventPersist("userConfigToken"),
  fromAction(RendererAction.LOGIN, "password").pipe(
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
  [userConfigToken, "userConfigToken"],
  [userAccessToken, "userAccessToken"],
  [userRefreshToken, "userRefreshToken"]
)
