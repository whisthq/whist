import { fetchBase } from "./fetchBase"
import { withCatch } from "./withCatch"
import { withGet } from "./withGet"
import { withJSON } from "./withJSON"
import { withPost } from "./withPost"
import { withStatus } from "./withStatus"
import { withServer } from "./withServer"
import { withURL } from "./withURL"
import { withTokenRefresh } from "./withTokenRefresh"
import { withHandleAuth } from "./withHandleAuth"
import { ConfigHTTP } from "../types/api"
import { decorate, decorateDebug } from "../utilities"
import { identity } from "lodash"

export const configGet = (c: ConfigHTTP) => {
  const decFn = c.debug ? decorateDebug : decorate

  return decFn(
    fetchBase,
    withGet,
    withCatch,
    withServer(c.server),
    withHandleAuth(c.handleAuth || identity),
    withTokenRefresh(c.endpointRefreshToken || ""),
    withURL,
    withStatus,
    withJSON
  )
}

export const configPost = (c: ConfigHTTP) => {
  const decFn = c.debug ? decorateDebug : decorate

  return decFn(
    fetchBase,
    withPost,
    withCatch,
    withServer(c.server),
    withHandleAuth(c.handleAuth || identity),
    withTokenRefresh(c.endpointRefreshToken || ""),
    withURL,
    withStatus,
    withJSON
  )
}
