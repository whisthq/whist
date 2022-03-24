import { from, merge, zip } from "rxjs"
import { switchMap, map, filter, tap } from "rxjs/operators"
import has from "lodash.has"

import { fork, flow } from "@app/main/utils/flows"
import {
  authInfoRefreshRequest,
  authInfoParse,
  isTokenExpired,
  refreshToken,
  accessToken,
  generateRandomConfigToken,
} from "@whist/core-ts"

export const authRefreshFlow = flow<refreshToken>(
  "authRefreshFlow",
  (trigger) => {
    const refreshed = fork(
      trigger.pipe(
        tap((x) => console.log("refresh flow", x)),
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

  const authInfo = merge(
    notExpired,
    refreshedAuthInfo.success,
    refreshedAuthInfo.failure
  )

  const withConfig = zip(authInfo, trigger).pipe(
    map(([a, t]) => ({
      ...a,
      configToken: t.configToken ?? generateRandomConfigToken(),
    }))
  )

  return {
    success: withConfig.pipe(filter((res) => !has(res, "error"))),
    failure: withConfig.pipe(filter((res) => has(res, "error"))),
  }
})
