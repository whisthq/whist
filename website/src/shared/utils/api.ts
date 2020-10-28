import { config } from "shared/constants/config"
import { debugLog } from "shared/utils/logging"
import history from "shared/utils/history"

// export async function apiPost(endpoint: any, body: any, token: any) {
//     try {
//         const response = await fetch(config.url.WEBSERVER_URL + endpoint, {
//             method: "POST",
//             mode: "cors",
//             headers: {
//                 "Content-Type": "application/json",
//                 Authorization: "Bearer " + token,
//             },
//             body: JSON.stringify(body),
//         })
//         const json = await response.json()
//         return { json, response }
//     } catch (err) {
//         debugLog(err)
//         return err
//     }
// }

async function sendPost(endpoint: string, body: any, token: string) {
    try {
        const response = await fetch(config.url.WEBSERVER_URL + endpoint, {
            method: "POST",
            mode: "cors",
            headers: {
                "Content-Type": "application/json",
                Authorization: "Bearer " + token,
            },
            body: JSON.stringify(body),
        })

        const json = await response.json()
        return { json, response }
    } catch (err) {
        return { json: null, response: { status: 500 } }
    }
}

export async function apiPost(
    endpoint: string,
    body: any,
    accessToken: string,
    refreshToken?: string
) {
    try {
        var { json, response } = await sendPost(endpoint, body, accessToken)
        if (
            json.error &&
            json.error === "Expired token" &&
            response.status === 401
        ) {
            if (!accessToken || accessToken === "") {
                history.push("/auth")
            }
            if (refreshToken && refreshToken !== "") {
                const refreshOutput = await sendPost(
                    "/token/refresh",
                    {},
                    refreshToken
                )
                const refreshJSON = refreshOutput.json
                if (refreshJSON) {
                    accessToken = refreshJSON.access_token
                }
                const output = await sendPost(endpoint, body, accessToken)
                json = output.json
                response = output.response
            }
        }
        return { json, response }
    } catch (err) {
        debugLog(err)
        return err
    }
}

export async function graphQLPost(
    operationsDoc: any,
    operationName: any,
    variables: any
) {
    try {
        const response = await fetch(config.url.GRAPHQL_HTTP_URL, {
            method: "POST",
            mode: "cors",
            headers: {
                "x-hasura-admin-secret": config.keys.HASURA_ACCESS_KEY,
            },
            body: JSON.stringify({
                query: stringifyGQL(operationsDoc),
                variables: variables,
                operationName: operationName,
            }),
        })
        const json = await response.json()
        return { json, response }
    } catch (err) {
        debugLog(err)
        return err
    }
}

export function stringifyGQL(doc: any) {
    return doc.loc && doc.loc.source.body
}
