import has from "lodash.has"

import {
  parseAuthInfo,
  requestAuthInfoRefresh,
  generateRandomConfigToken,
} from "@app/@core-ts/auth"
import { getStorage, setStorage } from "@app/worker/utils/storage"

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
  initConfigTokenHandler,
  authSuccess,
  authNetworkError,
  authFailure,
}
