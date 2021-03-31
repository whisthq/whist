import { identity } from "lodash"
import * as ctrl from "@app/main/controller"
import { logDebug } from "@app/utils/logging"

type DebugSchema = {
    [title: string]: [string] | [string, (...args: any[]) => any]
}

export const schema: DebugSchema = {
    appReady: ["event triggered"],
    appActivations: ["event triggered"],
    ipcStates: ["received state from renderer:", (state) => state],
    userLoginRequests: [
        "login requested with:",
        (req) => ({ ...req, password: "*********" }),
    ],
    userLoginLoading: ["loading is:", identity],
    userLoginResponses: ["login response:", identity],
    userLoginWarnings: ["login warning:", identity],
    persisted: ["loaded persisted state:", identity],
    userEmail: ["setting:", identity],
    userAccessToken: ["setting:", identity],
    containerCreateRequests: ["requesting with:", identity],
    containerCreateResponses: ["responded with:", identity],
    containerInfoReady: ["container ready?:", identity],
    containerInfoState: ["container state:", identity],
}

const ctrlModule = ctrl as any
if (process.env.DEBUG)
    for (let key in schema)
        ctrlModule[key] &&
            ctrlModule[key].subscribe((...args: any) => {
                let [message, subscribeFunc] = schema[key]
                let data = subscribeFunc ? subscribeFunc(...args) : undefined
                logDebug(key, message, data)
            })
