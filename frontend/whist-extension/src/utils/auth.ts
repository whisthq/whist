import { parseAuthInfo, requestAuthInfoRefresh } from "@app/@core-ts/auth"
import { getStorage } from "@app/utils/storage"

import { Storage } from "@app/constants/storage"

import { AuthInfo } from "@app/@types/payload"

const getCachedAuthInfo = async () => {
  const authInfo = (await getStorage(Storage.AUTH_INFO)) as any

  if (authInfo === undefined) return {}
  return JSON.parse(JSON.stringify(authInfo))
}

const refreshAuthInfo = async (authInfo: AuthInfo) => {
  const response = await requestAuthInfoRefresh(authInfo?.refreshToken)
  const json = await response.json()
  return parseAuthInfo(json)
}

export { getCachedAuthInfo, refreshAuthInfo }
