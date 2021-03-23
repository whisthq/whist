import path from "path"
import { spawn, ChildProcess } from "child_process"
import { app, screen } from "electron"
import { containerRequest, taskStatus } from "@app/utils/api"

const sleep = (ms: number) => new Promise((resolve) => setTimeout(resolve, ms))

const messages = {
    STARTING: "Initializing server connection.",
    PENDING: "Loading your browser.",
    FAILURE: "Oops! An unexpected error occured.",
    TIMEOUT:
        "Oops! Fractal took an unexpectedly long time to start. Please try again.",
}

const getProtocolName = () => {
    if (process.platform !== "darwin") return "Fractal.exe"
    if (app.isPackaged) return "./FractalClient"
    return "./Fractal"
}

// Temporarily pointing to the executable already installed in my applications
// folder so that I have something to launch.
// const protocolPath = path.join(app.getAppPath(), getProtocolName())
const protocolPath = "./protocol-build/client/Fractal"

const iconPath = path.join(app.getAppPath(), "build/icon64.png")

const getDPI = () => screen.getPrimaryDisplay().scaleFactor

const chooseRegion = () => {
    return [
        "us-east-1",
        "us-east-2",
        "us-west-1",
        "us-west-2",
        "ca-central-1",
        "eu-central-1",
    ][0]
}

export const parseInfoPorts = (res: {
    port_32262: number
    port_32263: number
    port_32273: number
}) => `32262:${res.port_32262}.32263:${res.port_32263}.32273:${res.port_32273}`

export const createContainer = async (email: string, accessToken: string) => {
    const r = containerRequest(email, accessToken, chooseRegion(), getDPI())
    const response = await r
    if (!response.json.ID)
        throw new Error(
            "Could not create container! Received: " +
                JSON.stringify(response, null, 4)
        )
    return response.json.ID
}

export const getContainerInfo = async (taskID: string, accessToken: string) => {
    const response = await taskStatus(taskID, accessToken)
    return response.json
}

export const waitUntilReady = async (taskID: string, accessToken: string) => {
    while (true) {
        let info = await getContainerInfo(taskID, accessToken)
        if (info.state === "SUCCESS") return info
        if (!(info.state === "PENDING" || info.state === "STARTED"))
            throw new Error(
                "Container startup failed! Received: " +
                    JSON.stringify(info, null, 4)
            )
        sleep(1000)
    }
}

export const launchProtocol = (info: {
    port_32262: number
    port_32263: number
    port_32273: number
    secret_key: string
    ip: string
}) => {
    if (process.platform === "darwin") spawn("chmod", ["+x", protocolPath])

    return spawn(
        protocolPath,
        [
            "--ports",
            parseInfoPorts(info),
            "--private-key",
            info.secret_key,
            info.ip,
        ],
        {
            detached: false,
            stdio: ["pipe", process.stdout, process.stderr],
        }
    )
}
