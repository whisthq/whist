import { fromEventPersist } from "@app/main/events/persist"
import { debugObservables } from "@app/utils/logging"
import { merge } from "rxjs"
import { identity } from "lodash"
import { share, filter } from "rxjs/operators"
import { formatObservable, formatTokens } from "@app/utils/formatters"

export const userSub = merge(fromEventPersist("sub")).pipe(
  filter(identity),
  share()
)

export const userAccessToken = fromEventPersist("accessToken").pipe(
  filter((token: string) => Boolean(token))
)

export const userRefreshToken = fromEventPersist("refreshToken").pipe(
  filter((token: string) => Boolean(token))
)

export const userConfigToken = fromEventPersist("userConfigToken").pipe(
  filter(identity),
  share()
)

// Logging
debugObservables(
  [userSub, "userSub"],
  [formatObservable(userConfigToken, formatTokens), "userConfigToken"],
  [formatObservable(userAccessToken, formatTokens), "userAccessToken"],
  [formatObservable(userRefreshToken, formatTokens), "userRefreshToken"]
)
