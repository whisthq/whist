import { configGet, configPost } from "@fractal/core-ts"
import { createConfigToken, decryptConfigToken } from "@app/utils/crypto"
import config from "@app/utils/config"
import { LoginServerError, LoginCredentialsWarning } from "@app/utils/errors"
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
    await post({
        endpoint: "/account/login",
        body: { username, password },
    })

export const responseAccessToken = (response: {
    json: { access_token?: string }
}) => response.json?.access_token

export const responseRefreshToken = (response: {
    json: { refresh_token?: string }
}) => response.json?.refresh_token

export const responseConfigToken = async (
    password: string,
    response: {
        json: { encrypted_config_token?: string }
    }
) =>
    response.json.encrypted_config_token
        ? decryptConfigToken(password, response.json.encrypted_config_token)
        : await createConfigToken()
export const tokenValidate = async (accessToken: string) =>
    get({ endpoint: "/token/validate", accessToken })

export const emailSignup = async (
    username: string,
    password: string,
    name: string,
    feedback: string
) =>
    await post({
        endpoint: "/account/register",
        body: { username, password, name, feedback },
    })

export const taskStatus = async (taskID: string, accessToken: string) =>
    get({ endpoint: "/status/" + taskID, accessToken })

export const containerRequest = async (
    username: string,
    accessToken: string,
    region: string,
    dpi: number
) =>
    post({
        endpoint: "/container/assign",
        accessToken,
        body: {
            username,
            region,
            dpi,
            app: "Google Chrome",
        },
    })
