/**
 * Copyright Fractal Computers, Inc. 2021
 * @file api.ts
 * @brief This file contains utility functions to make API calls.
 */

import { configGet, configPost } from "@fractal/core-ts"
import config from "@app/config/environment"
import https from "https"

/*
 * @fractal/core-ts http functions like "get" and "post"
 * are constructed at runtime so they can be passed a config
 * object, like the httpConfig object below.
 *
 * Naming across config objects will be consistent as core-ts grows.
 * Common prefixes will include:
 *
 * handle - A function that will accept a request or response object, to
 * be called for its side effects upon certain events, like error.
 *
 * endpoint - A string with the format "/api/endpoint", will be
 * concatenated onto a server prefix to form a complete URL.
 *
 */

const httpConfig = {
    server: config.url.WEBSERVER_URL,
    // handleAuth: (_: any) => goTo("/auth"),
    endpointRefreshToken: "/token/refresh",
}

export const get = configGet(httpConfig)
export const post = configPost(httpConfig)

export const apiPut = async (
    endpoint: string,
    server: string | undefined,
    body: Record<string, any>,
    ignoreCertificate = false
) => {
    /*
    Description:
        Sends an HTTP put request.
    NOTE: So far, we only make a PUT request when communicating with the host service, and the
        host service uses a self-signed certificate. `fetch` with a custom agent doesn't seem to work
        to `rejectUnauthorized`, so we use the lower-level `https` module and generate our own
        Promise to handle the response from the host service.
    Arguments:
        endpoint (string) : HTTP endpoint (e.g. /account/login)
        body (JSON) : PUT request body
        server (string) : HTTP URL (e.g. https://prod-server.fractal.co)
        ignoreCertificate (bool) : whether to ignore the endpoint host's certificate (used for self-signed).
            This is `false` by default because we only want to not `rejectUnauthorized` when we are certain
            about the host's certificate being self-signed or trusted.
    Returns:
        { json, success, response } (JSON) : Returned JSON of PUT request, success True/False, and HTTP response
    */

    const fullUrl = `${server ?? ""}${endpoint ?? ""}`

    return await new Promise((resolve, reject) => {
        // If we want to ignore the host certificate, then `rejectUnauthorized` should be false
        const request = https.request(
            fullUrl,
            {
                method: "PUT",
                headers: {
                    "Content-Type": "application/json",
                },
                rejectUnauthorized: !ignoreCertificate,
            },
            (response) => {
                let responseText = ""
                response.on("data", (data: string) => {
                    responseText = `${responseText}${data}`
                })
                response.on("end", () => {
                    const status = response?.statusCode ?? 400
                    const json = JSON.stringify(responseText)
                    resolve({ json, status, response })
                })
            }
        )
        request.write(JSON.stringify(body))
        request.on("error", (e) => {
            reject(e)
        })
        request.end()
    })
}

// TODO: this needs to move somewhere else, but we're not using it yet
export const tokenValidate = async (accessToken: string) =>
    get({ endpoint: "/token/validate", accessToken })
