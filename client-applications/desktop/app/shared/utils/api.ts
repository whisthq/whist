import { config } from "shared/constants/config"
import { debugLog } from "shared/utils/logging"
import {
    FractalHTTPRequest,
    FractalHTTPContent,
    FractalHTTPCode,
} from "shared/enums/api"

const checkResponse = (response: { status: number }): boolean => {
    return (
        response &&
        response.status &&
        (response.status === FractalHTTPCode.SUCCESS ||
            response.status === FractalHTTPCode.ACCEPTED)
    )
}

const checkJSON = <T>(json: T): boolean => {
    return json ? true : false
}

export const apiPost = async <T>(
    endpoint: string,
    body: T,
    token: string
): T => {
    try {
        const response = await fetch(config.url.WEBSERVER_URL + endpoint, {
            method: FractalHTTPRequest.POST,
            mode: "cors",
            headers: {
                "Content-Type": FractalHTTPContent.JSON,
                Authorization: "Bearer " + token,
            },
            body: JSON.stringify(body),
        })
        const json = await response.json()
        const success = checkJSON(json) && checkResponse(response)
        return { json, success }
    } catch (err) {
        debugLog(err)
        return err
    }
}

export const apiGet = async <T>(endpoint: string, token: string): T => {
    try {
        const response = await fetch(config.url.WEBSERVER_URL + endpoint, {
            method: FractalHTTPRequest.GET,
            mode: "cors",
            headers: {
                "Content-Type": FractalHTTPContent.JSON,
                Authorization: "Bearer " + token,
            },
        })
        const json = await response.json()
        const success = checkJSON(json) && checkResponse(response)
        return { json, success }
    } catch (err) {
        debugLog(err)
        return err
    }
}
