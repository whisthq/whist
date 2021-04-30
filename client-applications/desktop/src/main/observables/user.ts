import { fromEventIPC } from "@app/main/events/ipc"
import { fromEventPersist } from "@app/main/events/persist"
import { loginSuccess } from "@app/main/observables/login"
import { accessToken, refreshToken } from "@app/main/observables/login_new"
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
  tap
} from "rxjs/operators"
import {
  emailLoginConfigToken,
  emailLoginAccessToken,
  emailLoginRefreshToken,
} from "@app/utils/login"

export const userEmail = merge(
  fromEventPersist("email"),
).pipe(filter(identity), share())

export const userConfigToken = merge(
  fromEventPersist("userConfigToken"),
  fromEventIPC("loginRequest", "password").pipe( // TODO: remove
    sample(loginSuccess),
    withLatestFrom(loginSuccess),
    switchMap(([pw, res]) => from(emailLoginConfigToken(res, pw)))
  ),
  refreshToken // TODO: change this, currently just using refresh token as userConfigToken
).pipe(filter(identity), share())

export const userAccessToken = merge(
  accessToken
).pipe(filter(identity), share())

export const userRefreshToken = merge(
  fromEventPersist("userRefeshToken"),
  loginSuccess.pipe(map(emailLoginRefreshToken))
).pipe(filter(identity), share())

// Logging

debugObservables(
  [userEmail, "userEmail"],
  [userConfigToken, "userConfigToken"],
  [userAccessToken, "userAccessToken"],
  [userRefreshToken, "userRefreshToken"]
)
