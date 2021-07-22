/**
 * Copyright Fractal Computers, Inc. 2021
 * @file logging.ts
 * @brief This file contains utility functions for logging.
 */

import { app } from "electron"
import { truncate } from "lodash"
import fs from "fs"
import path from "path"
import util from "util"
import AWS from "aws-sdk"
import * as Amplitude from "@amplitude/node"

import { defaultAllowedRegions } from "@app/@types/aws"
import { chooseRegion } from "@app/utils/region"
import config, {
    loggingBaseFilePath,
    loggingFiles,
} from "@app/config/environment"
import { persistGet } from "@app/utils/persist"
import { sessionID } from "@app/utils/constants"

app.setPath("userData", loggingBaseFilePath)

const amplitude = Amplitude.init(config.keys.AMPLITUDE_KEY)
export const electronLogPath = path.join(loggingBaseFilePath, "logs")

// Variable to let us know whether the user's aws_region in Amplitude has been
// set for this session. We want it to only have to happen once, but we can't
// call `chooseRegion` up here because top level awaits aren't allowed.
let regionSet = false

// Open a file handle to append to the logs file.
// Create the loggingBaseFilePath directory if it does not exist.
const openLogFile = () => {
    fs.mkdirSync(electronLogPath, { recursive: true })
    const logPath = path.join(electronLogPath, loggingFiles.client)
    return fs.createWriteStream(logPath)
}

const logFile = openLogFile()

// Log levels
export enum LogLevel {
    DEBUG = "DEBUG",
    WARNING = "WARNING",
    ERROR = "ERROR",
}

const formatLogs = (title: string, data: object, level: LogLevel) => {
    // We use a special stringify function below before converting an object
    // to JSON. This is because certain objects in our application, like HTTP
    // responses and ChildProcess objects, have circular references in their
    // structure. This is normal NodeJS behavior, but it can cause a runtime error
    // if you blindly try to turn these objects into JSON. Our special stringify
    // function strips these circular references from the object.
    const template = `${level}: ${title} -- ${sessionID.toString()} -- \n ${util.inspect(
        data
    )}`

    const debugLog = truncate(template, {
        length: 1000,
        omission: "...**logBase only prints 1000 characters per log**",
    })

    return `${util.format(debugLog)} \n`
}

const localLog = (
    title: string,
    data: object,
    level: LogLevel,
    userEmail: string,
    msElapsed?: number
) => {
    const logs = formatLogs(
        `${title} -- ${userEmail} -- ${(msElapsed !== undefined
            ? msElapsed
            : 0
        ).toString()} ms since flow was triggered`,
        data,
        level
    )

    if (!app.isPackaged) console.log(logs)

    logFile.write(logs)
}

const amplitudeLog = async (
    title: string,
    data: object,
    userEmail: string,
    msElapsed?: number
) => {
    if (userEmail !== "") {
        if (!regionSet) {
            const region = await chooseRegion(defaultAllowedRegions)
            await amplitude.logEvent({
                event_type: `[${
                    (config.appEnvironment as string) ?? "LOCAL"
                }] ${title}`,
                session_id: sessionID,
                user_id: userEmail,
                event_properties: {
                    ...data,
                    msElapsed: msElapsed !== undefined ? msElapsed : 0,
                },
                user_properties: {
                    aws_region: region,
                },
            })
            regionSet = true
        } else {
            await amplitude.logEvent({
                event_type: `[${
                    (config.appEnvironment as string) ?? "LOCAL"
                }] ${title}`,
                session_id: sessionID,
                user_id: userEmail,
                event_properties: {
                    ...data,
                    msElapsed: msElapsed !== undefined ? msElapsed : 0,
                },
            })
        }
    }
}

export const logBase = (
    title: string,
    data: object,
    level: LogLevel,
    msElapsed?: number
) => {
    /*
  Description:
      Sends a log to console, client.log file, and/or logz.io depending on if the app is packaged
  Arguments:
      title (string): Log title
      data (any): JSON or list
      level (LogLevel): Log level, see enum LogLevel above
  */
    const userEmail = persistGet("userEmail") ?? ""
    localLog(title, data, level, userEmail as string, msElapsed)

    if (app.isPackaged)
        amplitudeLog(title, data, userEmail as string, msElapsed).catch((err) =>
            console.log(err)
        )
}

export const uploadToS3 = async () => {
    /*
  Description:
      Uploads a local file to S3
  Returns:
      Response from the s3 upload
  */
    if (!app.isPackaged) return

    const userEmail = (persistGet("userEmail") as string) ?? ""

    if (userEmail === "") return

    const s3FileName = `CLIENT_${userEmail}_${sessionID}.txt`

    const uploadHelper = async (localFilePath: string) => {
        const accessKey = config.keys.AWS_ACCESS_KEY
        const secretKey = config.keys.AWS_SECRET_KEY
        const bucketName = "fractal-protocol-logs"

        const s3 = new AWS.S3({
            accessKeyId: accessKey,
            secretAccessKey: secretKey,
        })
        // Read file into buffer
        const fileContent = fs.readFileSync(localFilePath)
        // Set up S3 upload parameters
        const params = {
            Bucket: bucketName,
            Key: s3FileName,
            Body: fileContent,
        }
        // Upload files to the bucket
        return await new Promise((resolve, reject) => {
            s3.upload(params, (err: Error, data: any) => {
                if (err !== null) {
                    reject(err)
                } else {
                    resolve(data)
                }
            })
        })
    }

    const logLocation = path.join(electronLogPath, loggingFiles.protocol)

    if (fs.existsSync(logLocation)) await uploadHelper(logLocation)
}
