import { app, BrowserWindow } from "electron"

import { ChildProcess } from "child_process"
import { FractalIPC } from "../shared/types/ipc"
import { launchProtocol, writeStream } from "../shared/utils/files/exec"
import { uploadToS3 } from "../shared/utils/files/aws"
import { FractalLogger } from "../shared/utils/general/logging"
import { FractalDirectory } from "../shared/types/client"
import LoadingMessage from "../pages/launcher/constants/loadingMessages"

/**
 * Function that launches the protocl from the main thread
 * @param mainWindow browser window of the client app, only referenced to kill the renderer thread
 */

const logger = new FractalLogger()

// export const createProtocol = (mainWindow: BrowserWindow | null = null) => {
//     const electron = require("electron")
//     const ipc = electron.ipcMain
//     let protocol: ChildProcess
//     let userID: string
//     let protocolLaunched: number
//     let protocolClosed: number
//     let createContainerRequestSent: number
// }

export const protocolOnStart = (userID: string) => {
    logger.logInfo("Protocol started, callback fired", userID)
}

export const protocolOnExit = (
    protocolLaunched: number,
    createContainerRequestSent: number,
    userID: string
) => {
    const protocolClosed = Date.now()
    const timer = {
        protocolLaunched,
        protocolClosed,
        createContainerRequestSent,
    }
    console.log("EXITING")
    logger.logInfo(
        `Protocol exited! Timer logs are ${JSON.stringify(timer)}`,
        userID
    )
    // For S3 protocol client log upload
    const logPath = require("path").join(
        FractalDirectory.getRootDirectory(),
        "protocol-build/desktop/log.txt"
    )

    const s3FileName = `CLIENT_${userID}_${new Date().getTime()}.txt`
    logger.logInfo(
        `Protocol client logs: https://fractal-protocol-logs.s3.amazonaws.com/${s3FileName}`,
        userID
    )

    // Upload client logs to S3 and shut down Electron
    uploadToS3(logPath, s3FileName, (s3Error: string) => {
        if (s3Error) {
            logger.logError(`Upload to S3 errored: ${s3Error}`, userID)
        }
    })

    // app.exit(0)
    // app.quit()
}
