import { app } from "electron"
import { uploadToS3 } from "../shared/utils/files/aws"
import { FractalLogger } from "../shared/utils/general/logging"
import { FractalDirectory } from "../shared/types/client"

const logger = new FractalLogger()

/**
 * Callback function called right before protocol starts
 * @param userID userID for logging purposes
 */
export const protocolOnStart = (userID: string) => {
    logger.logInfo("Protocol started, callback fired", userID)
}

/**
 * Callback function called right after protocol exits
 * @param protocolLaunched protocol launched timestamp for logging
 * @param createContainerRequestSent create container request timestamp for logging
 * @param userID userID for logging
 */
export const protocolOnExit = (
    protocolLaunched: number,
    createContainerRequestSent: number,
    userID: string,
    code: number,
    signal: string
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

    /*
          exit code:
            user exited: 0 
            termianl exited: 255
        
        exit signal: 
            SIGNINT: called from ipc listener 
    */

    logger.logInfo(
        `Protocol exited with signal ${signal}, code ${code}`,
        userID
    )

    if (signal !== "SIGINT" && code !== 255) {
        app.exit(0)
        app.quit()
    }
}
