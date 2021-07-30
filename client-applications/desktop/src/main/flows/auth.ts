import { from, merge } from "rxjs"
import { switchMap, map, filter } from "rxjs/operators"

import { flow, fork } from "@app/utils/flows"
import {
  generateRefreshedAuthInfo,
  generateRandomConfigToken,
  authInfoValid,
  isExpired,
} from "@app/utils/auth"
import { store } from "@app/utils/persist"

export const authRefreshFlow = flow<{
  refreshToken: string
}>("authRefreshFlow", (trigger) => {
  const refreshed = trigger.pipe(
    switchMap((tokens) => from(generateRefreshedAuthInfo(tokens.refreshToken)))
  )

  return fork(refreshed, {
    success: (result: any) => authInfoValid(result),
    failure: (result: any) => !authInfoValid(result),
  })
})

export default flow<{
  userEmail: string
  subClaim: string
  accessToken: string
  refreshToken: string
  configToken?: string
}>("authFlow", (trigger) => {
  const expired = trigger.pipe(
    filter((tokens) => isExpired(tokens.accessToken))
  )
  const notExpired = trigger.pipe(
    filter((tokens) => !isExpired(tokens.accessToken))
  )

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

  return fork(authInfoWithConfig, {
    success: (result: any) => authInfoValid(result),
    failure: (result: any) => !authInfoValid(result),
  })
})
