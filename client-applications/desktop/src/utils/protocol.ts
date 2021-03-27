import path from "path"
import { spawn, ChildProcess } from "child_process"
import { app, screen } from "electron"
import { containerRequest, taskStatus } from "@app/utils/api"
import { chooseRegion } from "@app/utils/aws"

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

const getDPI = () => screen.getPrimaryDisplay().scaleFactor * 72

export const parseInfoPorts = (res: {
    port_32262: number
    port_32263: number
    port_32273: number
}) => `32262:${res.port_32262}.32263:${res.port_32263}.32273:${res.port_32273}`

export const createContainer = async (email: string, accessToken: string) => {
    const region = await chooseRegion()
    const request = containerRequest(email, accessToken, region, getDPI())
    const response = await request
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

export const launchProtocolLoading = () => {
    if (process.platform === "darwin") spawn("chmod", ["+x", protocolPath])

    return spawn(protocolPath, ["--read-pipe"], {
        detached: false,
        stdio: ["pipe", process.stdout, process.stderr],
    })
}

export const writeStream = (process: ChildProcess, message: string) => {
    console.log(message)
    process.stdin?.write(message)
    process.stdin?.write("\n")
}

export const endStream = (process: ChildProcess, message: string) => {
    process.stdin?.end(message)
}

export const streamProtocolInfo = (
    protocol: ChildProcess,
    info: {
        port_32262: number
        port_32263: number
        port_32273: number
        secret_key: string
        ip: string
    }
) => {
    writeStream(protocol, `ports?${parseInfoPorts(info)}`)
    writeStream(protocol, `private-key?${info.secret_key}`)
    writeStream(protocol, `ip?${info.ip}`)
    writeStream(protocol, `finished?0`)
}

export const streamProtocolKill = (protocol: ChildProcess) => {
    writeStream(protocol, "kill?0")
    protocol.kill("SIGINT")
}
