import { config } from "shared/constants/config"
import { debugLog } from "shared/utils/logging"

export async function apiPost(endpoint: any, body: any, token: any, webserver_url: string=config.url.WEBSERVER_URL) {
    try {
        const response = await fetch(webserver_url + endpoint, {
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

export async function apiGet(endpoint: any, token: any, webserver_url: string=config.url.WEBSERVER_URL) {
    try {
        const response = await fetch(webserver_url + endpoint, {
            method: "GET",
            mode: "cors",
            headers: {
                "Content-Type": "application/json",
                Authorization: "Bearer " + token,
            },
        })
        const json = await response.json()
        return { json, response }
    } catch (err) {
        debugLog(err)
        return err
    }
}
