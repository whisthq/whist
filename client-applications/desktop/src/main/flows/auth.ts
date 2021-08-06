import { from, merge, zip } from "rxjs"
import { has } from "lodash"
import { switchMap, map, filter } from "rxjs/operators"
import { fork, flow } from "@app/utils/flows"
import {
  generateRandomConfigToken,
  authInfoRefreshRequest,
  authInfoParse,
  isTokenExpired,
  refreshToken,
  accessToken,
} from "@fractal/core-ts"
import { store } from "@app/utils/persist"

export const authRefreshFlow = flow<refreshToken>(
  "authRefreshFlow",
  (trigger) => {
    const refreshed = fork(
      trigger.pipe(
        switchMap((tokens: refreshToken) =>
          from(authInfoRefreshRequest(tokens))
        ),
        map(authInfoParse)
      ),
      {
        success: (res) => !has(res, "error"),
        failure: (res) => has(res, "error"),
      }
    )

    return {
      success: zip(refreshed.success, trigger).pipe(
        map(([r, t]) => ({ ...r, refreshToken: t.refreshToken }))
      ),
      failure: refreshed.failure,
    }
  }
)

export default flow<{
  userEmail: string
  accessToken: string
  refreshToken: string
  configToken?: string
}>("authFlow", (trigger) => {
  const expired = trigger.pipe(
    filter((tokens: accessToken & refreshToken) => isTokenExpired(tokens))
  )
  const notExpired = trigger.pipe(
    filter((tokens: accessToken) => !isTokenExpired(tokens))
  )

  const refreshedAuthInfo = authRefreshFlow(expired)

  const authInfoWithConfig = merge(
    notExpired,
    refreshedAuthInfo.success,
    refreshedAuthInfo.failure
  ).pipe(
    map(
      (tokens: {
        userEmail: string
        accessToken: string
        refreshToken: string
        configToken?: string
      }) => ({
        ...tokens,
        configToken:
          tokens.configToken ??
          store.get("auth.configToken") ??
          generateRandomConfigToken(),
      })
    )
  )

  return {
    success: authInfoWithConfig.pipe(filter((res) => !has(res, "error"))),
    failure: authInfoWithConfig.pipe(filter((res) => has(res, "error"))),
  }
})
