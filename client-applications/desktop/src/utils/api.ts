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

const graphqlConfig = {
    server: config.url.GRAPHQL_HTTP_URL,
    endpointRefreshToken: "/token/refresh",
}

export const get = configGet(httpConfig)

export const post = configPost(httpConfig)

export const emailLogin = async (username: string, password: string) =>
    post({ endpoint: "/account/login", body: { username, password } })

export const tokenValidate = async (accessToken: string) =>
    get({ endpoint: "/token/validate", accessToken })

export const emailSignup = async (
    username: string,
    password: string,
    name: string,
    feedback: string
) =>
    post({
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

const graphqlPost = configPost(graphqlConfig)

const stringifyGQL = (doc: any) => {
    return doc.loc && doc.loc.source.body
}

export const sendGraphql = async (
    operationsDoc: any,
    operationName: string,
    variables: any,
    accessToken?: string
) =>
    graphqlPost({
        endpoint: "",
        accessToken,
        body: JSON.stringify({
            query: stringifyGQL(operationsDoc),
            variables: variables,
            operationName: operationName,
        }),
    })
