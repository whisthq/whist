import { config } from "shared/constants/config"
import { debugLog } from "shared/utils/logging"

export async function apiPost(endpoint: any, body: any, token: any) {
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
        debugLog(err)
        return err
    }
}

export async function graphQLPost(operationsDoc: any, operationName: any, variables: any) {
    try {
        const response = await fetch(config.url.GRAPHQL_HTTP_URL, {
            method: 'POST',
            mode: "cors",
            headers: {
                "x-hasura-admin-secret": config.keys.HASURA_ACCESS_KEY,
            },
            body: JSON.stringify({
                "query": stringifyGQL(operationsDoc),
                "variables": variables,
                "operationName": operationName,
            }),
        })
        const json = await response.json()
        return { json, response }
    } catch(err) {
        debugLog(err)
        return err
    }
}

export function stringifyGQL(doc: any) {
    return doc.loc && doc.loc.source.body;
}