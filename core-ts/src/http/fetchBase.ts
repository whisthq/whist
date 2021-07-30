import {
    FractalHTTPContent,
    ServerResponse,
    ServerRequest,
    ServerEffect,
} from "../types/api"
import { isNode } from "browser-or-node"
import nodeFetch from "node-fetch"

// TODO: I don't think fractalBackoff() actually works as it's supposed to.
// Here's a way we can fix: https://www.npmjs.com/package/fetch-retry

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
            "Content-Type": FractalHTTPContent.JSON,
            Authorization: `Bearer ${req.token}`,
        },
        body: JSON.stringify(req.body),
    })

    return { request: req, response } as ServerResponse
}
