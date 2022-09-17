import has from "lodash.has"

import {
  parseAuthInfo,
  requestAuthInfoRefresh,
  authInfoCallbackRequest,
  generateRandomConfigToken,
} from "@app/@core-ts/auth"
import { getStorage, setStorage } from "@app/worker/utils/storage"

import { config } from "@app/constants/app"
import { Storage } from "@app/constants/storage"
import { AuthInfo, ConfigTokenInfo } from "@app/@types/payload"

const initWhistAuth = async () => {
  /*
    Description:
      Checks cached auth tokens, refreshes them, and prompts the user to re-authenticate
      if necessary.
  */

  const cachedAuthInfo = await getStorage<AuthInfo>(Storage.AUTH_INFO)
  const refreshResponse = await requestAuthInfoRefresh(
    cachedAuthInfo?.refreshToken ?? ""
  )
  const refreshedAuthInfo = {
    ...parseAuthInfo(refreshResponse),
    isFirstAuth: false,
  }

  return refreshedAuthInfo
}

const initGoogleAuthHandler = async () => {
  /*
    Description:
      Opens the Google auth window when requested by the user
  */
  const authPromise = new Promise<AuthInfo>((resolve) => {
    chrome.webRequest.onBeforeRequest.addListener(
      (details: { url: string }) => {
        const callbackUrl = details.url

        void authInfoCallbackRequest(callbackUrl).then((response: any) => {
          const authInfo = parseAuthInfo(response)
          const refreshToken = response?.json.refresh_token
          resolve({
            ...authInfo,
            refreshToken,
            isFirstAuth: true,
          })
        })
      },
      {
        urls: [`${<string>config.AUTH0_REDIRECT_URL}*`],
      },
      ["blocking"]
    )
  })

  return await authPromise
}

const initConfigTokenHandler = async () => {
  const configTokenInfo = await getStorage<ConfigTokenInfo>(
    Storage.CONFIG_TOKEN_INFO
  )

  if (configTokenInfo?.token === undefined) {
    void setStorage(Storage.CONFIG_TOKEN_INFO, {
      token: generateRandomConfigToken(),
      isNew: true,
    })
  } else {
    void setStorage(Storage.CONFIG_TOKEN_INFO, {
      ...configTokenInfo,
      isNew: false,
    })
  }
}

const authSuccess = (payload: AuthInfo) => {
  return payload !== undefined && !has(payload, "error")
}

const authNetworkError = (payload: AuthInfo) => {
  return (
    payload !== undefined &&
    has(payload, "error") &&
    payload.error?.data?.status === 500
  )
}

const authFailure = (payload: AuthInfo) => {
  return (
    payload !== undefined &&
    has(payload, "error") &&
    payload.error?.data?.status !== 500
  )
}

export {
  initWhistAuth,
  initGoogleAuthHandler,
  initConfigTokenHandler,
  authSuccess,
  authNetworkError,
  authFailure,
}
