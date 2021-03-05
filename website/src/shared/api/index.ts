import fetch from "node-fetch"

import { configGet, configPost } from "@fractal/core-ts/http"
import { config } from "shared/constants/config"
import history from "shared/utils/history"

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

/*
    TODO: Replace hard-coded endpoints with FractalAPI type
    TODO: Split this file into different files e.g. auth.ts, payment.ts, etc.
    https://github.com/fractal/website/issues/338
*/

const httpConfig = {
    server: config.url.WEBSERVER_URL,
    handleAuth: (_: any) => history.push("/auth"),
    endpointRefreshToken: "/token/refresh",
}

const get = configGet(httpConfig)

const post = configPost(httpConfig)

export const loginEmail = async (username: string, password: string) =>
    post({ endpoint: "/account/login", body: { username, password } })

export const loginGoogle = async (code: any) =>
    post({ endpoint: "/google/login", body: { code } })

export const signupEmail = async (
    username: string,
    password: string,
    name: string,
    feedback: string
) =>
    /*
        API call to the /account/register endpoint to register a new email

        Arguments:
            username (string): email of the new user
            password (string): password of the new user
            name (string): name of the new user
            feedback (string): feedback from the new user (deprecated - currently 
                sending an empty string)
        Returns:
            { json, success } (JSON) : Returned JSON of POST request and success True/False
    */
    post({
        endpoint: "/account/register",
        body: { username, password, name, feedback },
    })

export const emailVerification = async (
    username: string,
    name: string,
    token: string
) => {
    /*
        API call to the /mail endpoint to send a verification email

        Arguments:
            username (string): email of the user
            name (string): name of the user
            token (string): email verification token of the user
        
        Returns:
            { json, success } (JSON) : Returned JSON of POST request and success True/False
    */
    const body = {
        email_id: "EMAIL_VERIFICATION",
        to_email: username,
        email_args: {
            name: name,
            link: config.url.FRONTEND_URL + "/verify?" + token,
        },
    }
    return post({
        endpoint: "/mail",
        body,
    })
}

export const validateVerification = async (
    accessToken: string,
    refreshToken: string,
    username: string,
    token: string
) =>
    post({
        endpoint: "/account/verify",
        body: { username, token },
        accessToken,
        refreshToken,
    })

export const deleteAccount = async (username: string, accessToken: string) => {
    post({
        endpoint: "/account/delete",
        body: { username },
        accessToken,
    })
}

export const passwordForgot = async (username: string, emailToken: string) => {
    /*
        API call to the /mail endpoint to send a password forgot email

        Arguments:
            username (string): email of the user
            token (string): email verification token of the user
        
        Returns:
            { json, success } (JSON) : Returned JSON of POST request and success True/False
    */

    //TODO: is this emailToken different from i.e. token in emailVerification?
    // https://github.com/fractal/website/issues/338
    const body = {
        email_id: "PASSWORD_RESET",
        to_email: username,
        email_args: {
            link: config.url.FRONTEND_URL + "/reset?" + emailToken,
        },
    }
    return post({
        endpoint: "/mail",
        body,
    })
}

export const validatePasswordReset = async (token: string) =>
    get({ endpoint: "/token/validate", accessToken: token })

export const passwordReset = async (
    token: string,
    username: string,
    password: string
) =>
    post({
        endpoint: "/account/update",
        body: { username, password },
        accessToken: token,
    })

export const passwordUpdate = async (
    accessToken: string,
    username: string,
    password: string
) =>
    post({
        endpoint: "/account/verify_password",
        body: { username, password },
        accessToken,
    })

export const stripePaymentInfo = async (accessToken: string, email: string) =>
    post({
        endpoint: "/stripe/retrieve",
        body: { email },
        accessToken,
    })

export const paymentCardAdd = async (
    accessToken: string,
    refreshToken: string,
    email: string,
    source: string
) =>
    post({
        endpoint: "/stripe/addCard",
        body: { email, source },
        accessToken,
        refreshToken,
    })

export const paymentCardDelete = async (
    accessToken: string,
    refreshToken: string,
    email: string,
    source: string
) =>
    post({
        endpoint: "/stripe/deleteCard",
        body: { email, source },
        accessToken,
        refreshToken,
    })

export const addSubscription = async (
    accessToken: string,
    refreshToken: string,
    email: string,
    plan: string
) =>
    post({
        endpoint: "/stripe/addSubscription",
        body: { email, plan },
        accessToken,
        refreshToken,
    })

export const cancelMail = async (username: string, feedback: string) => {
    /*
        API call to the /mail endpoint to send an email notifying us that a user 
        has canceled their plan

        Arguments:
            username (string): email of the user
            feedback (string): feedback from the user about why they canceled
        
        Returns:
            { json, success } (JSON) : Returned JSON of POST request and success True/False
    */
    const body = {
        email_id: "FEEDBACK",
        to_email: "support@fractal.co",
        email_args: {
            user: username,
            feedback: feedback,
        },
    }
    return post({
        endpoint: "/mail",
        body,
    })
}

export const deleteSubscription = async (
    accessToken: string,
    refreshToken: string,
    email: string
) =>
    post({
        endpoint: "/stripe/deleteSubscription",
        body: { email },
        accessToken,
        refreshToken,
    })

export const stringifyGQL = (doc: any) => {
    return doc.loc && doc.loc.source.body
}

export const graphQLPost = async (
    operationsDoc: any,
    operationName: string,
    variables: any,
    accessToken?: string,
    admin = false
) => {
    /*
    Description:
        Sends an GraphQL POST request

    Arguments:
        operationsDoc (any) : GraphQL query
        operationName (string) : Name of GraphQL query
        variables (string) : Variables passed into query
        accessToken (string) : Access token

    Returns:
        { json, success } (JSON) : Returned JSON of GET request and success True/False
    */

    let headers = {}
    if (!admin) {
        headers = {
            Authorization: accessToken ? `Bearer ${accessToken}` : `Bearer`,
        }
    } else {
        headers = {
            "x-hasura-admin-secret": accessToken,
        }
    }

    try {
        const response = await fetch(config.url.GRAPHQL_HTTP_URL, {
            method: "POST",
            headers: headers,
            body: JSON.stringify({
                query: stringifyGQL(operationsDoc),
                variables: variables,
                operationName: operationName,
            }),
        })
        const json = await response.json()
        return { json, response }
    } catch (err) {
        return err
    }
}
