import { fromEventIPC } from "@app/main/events/ipc"
import { fromEventPersist } from "@app/main/events/persist"
import { loginSuccess } from "@app/main/observables/login"
import { accessToken, refreshToken } from "@app/main/observables/login_new"
import { debugObservables } from "@app/utils/logging"
import { merge, from } from "rxjs"
import { identity } from "lodash"
import {
  share,
  filter,
  tap
} from "rxjs/operators"
import { checkJWTExpired } from "@app/utils/auth0"

export const userEmail = merge(
  fromEventPersist("email"),
).pipe(filter(identity), share())

export const userConfigToken = merge(
  fromEventPersist("userConfigToken"),
  refreshToken // TODO: change this, currently just using refresh token as userConfigToken
).pipe(filter(identity), share())

export const userAccessToken = fromEventPersist("accessToken").pipe(
  filter((token: string) => !!token),
  filter((token: string) => !checkJWTExpired(token))
)

export const userRefreshToken = fromEventPersist("refreshToken").pipe(
  filter((token: string) => !!token)
)

// Logging

debugObservables(
  [userEmail, "userEmail"],
  [userConfigToken, "userConfigToken"],
  [userAccessToken, "userAccessToken"],
  [userRefreshToken, "userRefreshToken"]
)
