import { loadTokens, refreshTokens } from "@app/main/events/auth"
import { merge, zip } from "rxjs"
import { filter } from "rxjs/operators"
import {
  userAccessToken,
  userRefreshToken,
  userConfigToken,
  userSub,
} from "@app/main/observables/user"
import { debugObservables } from "@app/utils/logging"
import { checkJWTExpired } from "@app/utils/auth"

const tokenUpdate = merge(loadTokens, refreshTokens)

export const tokenUpdateSuccess = tokenUpdate.pipe(
  filter((data) => (data?.sub ?? "") !== "")
)

export const tokenUpdateFailure = tokenUpdate.pipe(
  filter((data) => (data?.sub ?? "") === "")
)

export const authSuccess = zip(
  userAccessToken,
  userRefreshToken,
  userConfigToken,
  userSub
).pipe(filter(([accessToken]) => !checkJWTExpired(accessToken)))
