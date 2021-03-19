import fetch from "node-fetch"

import { configGet, configPost } from "@fractal/core-ts"
import config from "@app/utils/config"

import { goTo } from "@app/utils/history"

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

export const emailLogin = async (username: string, password: string) =>
    post({ endpoint: "/account/login", body: { username, password } })

export const tokenValidate = async (accessToken: string) =>
    get({ endpoint: "/token/validate", accessToken })
