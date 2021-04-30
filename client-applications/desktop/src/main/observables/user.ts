import { fromEventPersist } from "@app/main/events/persist"
import { debugObservables } from "@app/utils/logging"
import { merge } from "rxjs"
import { identity } from "lodash"
import {
  share,
  filter
} from "rxjs/operators"
import { checkJWTExpired } from "@app/utils/auth"
import { formatObservable, formatTokens } from "@app/utils/formatters"

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
  [formatObservable(userConfigToken, formatTokens), "userConfigToken"],
  [formatObservable(userAccessToken, formatTokens), "userAccessToken"],
  [formatObservable(userRefreshToken, formatTokens), "userRefreshToken"]
)
