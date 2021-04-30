import { fromEventPersist } from "@app/main/events/persist"
import { debugObservables } from "@app/utils/logging"
import { merge } from "rxjs"
import { identity } from "lodash"
import {
  share,
  filter,
  tap
} from "rxjs/operators"
import { checkJWTExpired } from "@app/utils/auth"

export const userEmail = merge(
  fromEventPersist("email"),
).pipe(filter(identity), share())

export const userAccessToken = fromEventPersist("accessToken").pipe(
  filter((token: string) => !!token),
  filter((token: string) => !checkJWTExpired(token))
)

export const userRefreshToken = fromEventPersist("refreshToken").pipe(
  filter((token: string) => !!token)
)

export const userConfigToken = merge(
  fromEventPersist("userConfigToken"),
  userRefreshToken // TODO: change this, currently just using refresh token as userConfigToken to test boot
).pipe(filter(identity), share())

// Logging

debugObservables(
  [userEmail, "userEmail"],
  [userConfigToken, "userConfigToken"],
  [userAccessToken, "userAccessToken"],
  [userRefreshToken, "userRefreshToken"]
)
