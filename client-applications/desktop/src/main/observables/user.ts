import { fromEventIPC } from "@app/main/events/ipc"
import { fromEventPersist } from "@app/main/events/persist"
import { loginSuccess } from "@app/main/observables/login"
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

export const userEmail = merge(
  fromEventPersist("userEmail"),
  fromEventIPC("loginRequest", "email").pipe(sample(loginSuccess)),
).pipe(filter(identity), share())

export const userConfigToken = merge(
  fromEventPersist("userConfigToken"),
  fromEventIPC("loginRequest", "password").pipe(
    sample(loginSuccess),
    withLatestFrom(loginSuccess),
    switchMap(([pw, res]) => from(emailLoginConfigToken(res, pw)))
  )
).pipe(filter(identity), share())

export const userAccessToken = merge(
  fromEventPersist("userAccessToken"),
  loginSuccess.pipe(map(emailLoginAccessToken))
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
