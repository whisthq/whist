import has from "lodash.has"

import { parseAuthInfo, requestAuthInfoRefresh } from "@app/@core-ts/auth"
import { getStorage } from "@app/worker/utils/storage"

import { Storage } from "@app/constants/storage"
import { AuthInfo } from "@app/@types/payload"

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

export { initWhistAuth, authSuccess, authNetworkError, authFailure }
