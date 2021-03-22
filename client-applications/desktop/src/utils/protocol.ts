import path from "path"
import { spawn, ChildProcess } from "child_process"
import { app, screen } from "electron"
import { containerRequest, taskStatus } from "@app/utils/api"

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

const protocolPath = path.join(app.getAppPath(), getProtocolName())

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

export const writeStream = (process: ChildProcess, message: string) => {
    process.stdin?.write("")
    process.stdin?.write("\n")
}

export const endStream = (process: ChildProcess, message: string) => {
    process.stdin?.end(message)
}

export const launchProtocol = () => {
    if (process.platform === "darwin")
        spawn("chmod", ["+x", app.isPackaged ? "Fractal" : "FractalClient"])

    spawn(
        protocolPath,
        ["--name", "Fractalized Chrome", "--icon", iconPath, "--read-pipe"],
        {
            detached: false,
            stdio: ["pipe", process.stdout, process.stderr],
        }
    )
}

export const getContainerInfo = async (taskID: string, accessToken: string) => {
    const response = await taskStatus(taskID, accessToken)
    return response.json
}

export const createContainer = async (
    username: string,
    accessToken: string
) => {
    return await containerRequest(
        username,
        accessToken,
        chooseRegion(),
        getDPI()
    )
}

// Still need to pass in portInfo and container
export const signalProtocolInfo = (protocol: ChildProcess, info: { ports }) => {
    writeStream(protocol, `ports?${portInfo}`)
    writeStream(protocol, `private-key?${container.secretKey}`)
    writeStream(protocol, `ip?${container.publicIP}`)
    writeStream(protocol, `finished?0`)
}

export const signalProtocolLoading = (protocol: ChildProcess) => {
    writeStream(protocol, `loading?${messages.PENDING}`)
}

export const signalProtocolKill = (protocol: ChildProcess) => {
    writeStream(protocol, "kill?0")
    protocol.kill("SIGINT")
}
