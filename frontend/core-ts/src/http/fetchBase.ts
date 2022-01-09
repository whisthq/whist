import {
  WhistHTTPContent,
  ServerResponse,
  ServerRequest,
  ServerEffect,
} from "../types/api"
import { isNode } from "browser-or-node"
import nodeFetch from "node-fetch"
import https from "https"

const agent = new https.Agent({
  rejectUnauthorized: false,
})

/*
 * Performs an HTTP request. Must be passed url and method
 * keys. Not intended for direct usage, but as a base function
 * to be wrapped in a decorator pipeline.
 *
 * The request object is contained within the return value.
 *
 * @param req - a ServerRequest object
 * @returns a ServerResponse wrapped in a Promise
 */
export const fetchBase: ServerEffect = async (req: ServerRequest) => {
  const fetchFunc = isNode ? nodeFetch : fetch

  const response = await fetchFunc(req.url || "", {
    method: req.method || "",
    // mode: "cors",
    headers: {
      "Content-Type": WhistHTTPContent.JSON,
      Authorization: `Bearer ${req.token}`,
    },
    body: JSON.stringify(req.body),
    // TODO: Once Electron fixes https://github.com/electron/electron/issues/31212
    // we can get rid of this
    ...((req.url ?? "").startsWith("https") && { agent }),
  })

  return { request: req, response } as ServerResponse
}
