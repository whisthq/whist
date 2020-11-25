import { config, webservers } from "shared/constants/config"
import { debugLog } from "shared/utils/logging"

// if a webserver is passed and it's none then this is meant to fail

export async function apiPost(
    endpoint: any,
    body: any,
    token: any,
    webserver: string = config.url.WEBSERVER_URL
) {
    const webserver_url =
        webserver in webservers ? webservers[webserver] : webserver

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

export async function apiGet(
    endpoint: any,
    token: any,
    webserver: string = config.url.WEBSERVER_URL
) {
    const webserver_url =
        webserver in webservers ? webservers[webserver] : webserver

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
