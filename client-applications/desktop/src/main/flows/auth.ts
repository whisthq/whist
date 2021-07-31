import { from, merge } from "rxjs"
import { has } from "lodash"
import { switchMap, map, filter } from "rxjs/operators"
import { flow } from "@app/utils/flows"
import {
  generateRandomConfigToken,
  authInfoRefreshRequest,
  authInfoParse,
  isTokenExpired,
} from "@fractal/core-ts"
import { store } from "@app/utils/persist"

export const authRefreshFlow = flow<{
  refreshToken: string
}>("authRefreshFlow", (trigger) => {
  const refreshed = trigger.pipe(
    switchMap((tokens) => from(authInfoRefreshRequest(tokens))),
    map(authInfoParse)
  )

  return {
    success: refreshed.pipe(filter((res) => !has(res, "error"))),
    failure: refreshed.pipe(filter((res) => has(res, "error"))),
  }
})

export default flow<{
  userEmail: string
  accessToken: string
  refreshToken: string
  configToken?: string
}>("authFlow", (trigger) => {
  const expired = trigger.pipe(filter((tokens) => isTokenExpired(tokens)))
  const notExpired = trigger.pipe(filter((tokens) => !isTokenExpired(tokens)))

  const refreshedAuthInfo = authRefreshFlow(expired)

  const authInfoWithConfig = merge(
    notExpired,
    refreshedAuthInfo.success,
    refreshedAuthInfo.failure
  ).pipe(
    map((tokens) => ({
      ...tokens,
      configToken:
        tokens.configToken ??
        store.get("configToken") ??
        generateRandomConfigToken(),
    }))
  )

  return {
    success: authInfoWithConfig.pipe(filter((res) => !has(res, "error"))),
    failure: authInfoWithConfig.pipe(filter((res) => has(res, "error"))),
  }
})
