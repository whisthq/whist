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

// When a response is received from Auth0
const tokenUpdate = merge(loadTokens, refreshTokens)

// Valid tokens were received from Auth0
export const tokenUpdateSuccess = tokenUpdate.pipe(
  filter((data) => (data?.sub ?? "") !== "")
)

// Could not receive tokens from Auth0
export const tokenUpdateFailure = tokenUpdate.pipe(
  filter((data) => (data?.sub ?? "") === "")
)

// Successful authentication occurred
export const authSuccess = zip(
  userAccessToken,
  userRefreshToken,
  userConfigToken,
  userSub
).pipe(filter(([accessToken]) => !checkJWTExpired(accessToken)))
