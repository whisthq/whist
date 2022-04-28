import {
  parseAuthInfo,
  requestAuthInfoRefresh,
} from "@app/worker/@core-ts/auth"
import { getStorage } from "@app/worker/utils/storage"

import { cachedAuthInfo } from "@app/constants/storage"

const getCachedAuthInfo = async () => {
  const _authInfo = (await getStorage(cachedAuthInfo)) as any

  if (_authInfo === undefined) return {}
  return JSON.parse(JSON.stringify(_authInfo))
}

const refreshAuthInfo = async (authInfo: any) => {
  const response = await requestAuthInfoRefresh(authInfo?.refreshToken)
  const json = await response.json()
  return parseAuthInfo(json)
}

export { getCachedAuthInfo, refreshAuthInfo }
