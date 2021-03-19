import { config, webservers } from "shared/constants/config"
import { debugLog } from "shared/utils/general/logging"
import {
    FractalHTTPRequest,
    FractalHTTPContent,
    FractalHTTPCode,
} from "shared/types/api"
import { fractalBackoff } from "shared/utils/general/helpers"
import fetch from "node-fetch"
import https from "https"

const checkResponse = (response: { status: number }): boolean => {
    /*
    Description:
        Reads HTTP response and checks if it is successful

    Arguments:
        response (JSON): HTTP response

    Returns:
        success (boolean) : True/false
    */
    if (
        response &&
        response.status &&
        (response.status === FractalHTTPCode.SUCCESS ||
            response.status === FractalHTTPCode.ACCEPTED)
    ) {
        return true
    }
    return false
}

const checkJSON = (json: Record<string, any>): boolean => {
    /*
    Description:
        Reads a JSON and checks if it is undefined

    Arguments:
        json (JSON): Any JSON object

    Returns:
        success (boolean) : True if not undefined, false otherwise
    */
    return !!json
}

export const apiPost = async (
    endpoint: string,
    body: Record<string, any>,
    token: string,
    webserver: string | undefined = config.url.WEBSERVER_URL
) => {
    /*
    Description:
        Sends an HTTP POST request

    Arguments:
        endpoint (string) : HTTP endpoint (e.g. /account/login)
        body (JSON) : POST request body
        token (string) : Access token
        webserver (string) : HTTP URL (e.g. https://prod-server.fractal.co)

    Returns:
        { json, success, response } (JSON) : Returned JSON of POST request, success True/False, and HTTP response
    */
    if (webserver) {
        const webserverUrl =
            webserver in webservers ? webservers[webserver] : webserver

        try {
            const fullUrl = `${webserverUrl}${endpoint}`
            const response = await fractalBackoff(() =>
                fetch(fullUrl, {
                    method: FractalHTTPRequest.POST,
                    headers: {
                        "Content-Type": FractalHTTPContent.JSON,
                        Authorization: `Bearer ${token}`,
                    },
                    body: JSON.stringify(body),
                })
            )
            const json = await response.json()
            const success = checkJSON(json) && checkResponse(response)
            return { json, success, response }
        } catch (err) {
            debugLog(err)
            return err
        }
    } else {
        return { json: null, success: false, response: null }
    }
}

export const apiGet = async (
    endpoint: string,
    token: string,
    webserver: string | undefined = config.url.WEBSERVER_URL
) => {
    /*
    Description:
        Sends an HTTP GET request

    Arguments:
        endpoint (string) : HTTP endpoint (e.g. /account/login)
        token (string) : Access token
        webserver (string) : HTTP URL (e.g. https://prod-server.fractal.co)

    Returns:
        { json, success } (JSON) : Returned JSON of GET request and success True/False
    */
    if (webserver) {
        const webserverUrl =
            webserver in webservers ? webservers[webserver] : webserver

        try {
            const fullUrl = `${webserverUrl}${endpoint}`
            const response = await fractalBackoff(() =>
                fetch(fullUrl, {
                    method: FractalHTTPRequest.GET,
                    headers: {
                        "Content-Type": FractalHTTPContent.JSON,
                        Authorization: `Bearer ${token}`,
                    },
                })
            )
            const json = await response.json()
            const success = checkJSON(json) && checkResponse(response)
            return { json, success }
        } catch (err) {
            debugLog(err)
            return err
        }
    } else {
        return { json: null, success: false }
    }
}

export const apiDelete = async (
    endpoint: string,
    token: string,
    webserver: string | undefined = config.url.WEBSERVER_URL
) => {
    /*
    Description:
        Sends an HTTP DELETE request

    Arguments:
        endpoint (string) : HTTP endpoint (e.g. /account/login)
        token (string) : Access token
        webserver (string) : HTTP URL (e.g. https://prod-server.fractal.co)

    Returns:
        { json, success } (JSON) : Returned JSON of GET request and success True/False
    */
    if (webserver) {
        const webserverUrl =
            webserver in webservers ? webservers[webserver] : webserver

        try {
            const fullUrl = `${webserverUrl}${endpoint}`
            const response = await fractalBackoff(() =>
                fetch(fullUrl, {
                    method: FractalHTTPRequest.DELETE,
                    headers: {
                        "Content-Type": FractalHTTPContent.JSON,
                        Authorization: `Bearer ${token}`,
                    },
                })
            )
            const json = await response.json()
            const success = checkJSON(json) && checkResponse(response)
            return { json, success }
        } catch (err) {
            debugLog(err)
            return err
        }
    } else {
        return { json: null, success: false }
    }
}

export const stringifyGQL = (doc: any) => {
    return doc.loc && doc.loc.source.body
}

export const graphQLPost = async (
    operationsDoc: any,
    operationName: string,
    variables: any,
    accessToken?: string
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

    try {
        const response = await fractalBackoff(() =>
            fetch(config.url.GRAPHQL_HTTP_URL, {
                method: "POST",
                headers: {
                    Authorization: accessToken
                        ? `Bearer ${accessToken}`
                        : `Bearer`,
                },
                body: JSON.stringify({
                    query: stringifyGQL(operationsDoc),
                    variables: variables,
                    operationName: operationName,
                }),
            })
        )
        const json = await response.json()
        return { json, response }
    } catch (err) {
        debugLog(err)
        return err
    }
}

export const apiPut = async (
    endpoint: string,
    body: Record<string, any>,
    server: string | undefined,
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

    if (server) {
        try {
            const fullUrl = `${server}${endpoint}`

            const returnDict = await fractalBackoff(() => {
                return new Promise((resolve, reject) => {
                    // If we want to ignore the host certificate, then `rejectUnauthorized` should be false
                    const request = https.request(
                        fullUrl,
                        {
                            method: "PUT",
                            headers: {
                                "Content-Type": FractalHTTPContent.JSON,
                            },
                            rejectUnauthorized: !ignoreCertificate,
                        },
                        (response) => {
                            let responseText = ""
                            response.on("data", (data) => {
                                responseText += data
                            })
                            response.on("end", () => {
                                const statusCode = response.statusCode
                                    ? response.statusCode
                                    : 400
                                const json = JSON.stringify(responseText)
                                const success = checkResponse({
                                    status: statusCode,
                                })
                                resolve({ json, success, response: statusCode })
                            })
                        }
                    )
                    request.write(JSON.stringify(body))
                    request.on("error", (e) => {
                        reject(e)
                    })
                    request.end()
                })
            })
            return returnDict
        } catch (err) {
            debugLog(err)
            return err
        }
    } else {
        return { json: null, success: false, response: null }
    }
}
