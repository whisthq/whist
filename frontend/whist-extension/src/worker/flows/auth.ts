import has from "lodash.has"
import isEmpty from "lodash.isEmpty"

import { setStorage } from "@app/worker/utils/storage"
import {
  isTokenExpired,
  authInfoParse,
  authInfoRefreshRequest,
  getCachedAuthInfo,
  authPortalURL,
  authInfoCallbackRequest,
} from "@app/worker/utils/auth"
import { createAuthTab } from "@app/worker/utils/tabs"

import { CACHED_AUTH_INFO } from "@app/constants/storage"

const shouldRefresh = async (authInfo: any) => {
  const isExpired = isTokenExpired(authInfo?.accessToken ?? "")
  return isExpired && authInfo?.refreshToken !== undefined
}

const refreshFlow = async (authInfo: any) => {
  const response = await authInfoRefreshRequest(authInfo?.refreshToken)
  const json = await response.json()
  return authInfoParse(json)
}

const authenticateCachedAuthInfo = async () => {
  let authInfo = await getCachedAuthInfo()

  if (await shouldRefresh(authInfo)) {
    authInfo = await refreshFlow(authInfo)

    if (has(authInfo, "error")) return false

    await setStorage(
      CACHED_AUTH_INFO,
      JSON.stringify({
        userEmail: authInfo.userEmail,
        accessToken: authInfo.accessToken,
        refreshToken: authInfo?.refreshToken,
      })
    )
  }

  return true
}

const googleAuth = () => {
  chrome.identity.launchWebAuthFlow(
    {
      url: authPortalURL(),
      interactive: true,
    },
    async (callbackUrl) => {
      const response = await authInfoCallbackRequest(callbackUrl ?? "")
      const json = await response.json()
      const authInfo = authInfoParse(json)

      await setStorage(
        CACHED_AUTH_INFO,
        JSON.stringify({
          userEmail: authInfo.userEmail,
          accessToken: authInfo.accessToken,
          refreshToken: json?.json.refreshToken,
        })
      )
    }
  )
}

const authenticate = async () => {
  const wasLoggedIn = isEmpty(await getCachedAuthInfo())

  if (!wasLoggedIn) {
    createAuthTab()
  } else {
    const authenticated = authenticateCachedAuthInfo()
    if (!authenticated) createAuthTab()
  }
}

export { googleAuth, authenticate }
