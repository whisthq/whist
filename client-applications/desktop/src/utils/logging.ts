import { app } from "electron"
import fs from "fs"
import util from "util"
import path from "path"
import AWS from "aws-sdk"
import os from "os"

import config from "@app/utils/config"

// Where to send .log file
const logPath = path.join(
    app.getAppPath().replace("/Resources/app.asar", ""),
    "debug.log"
)
const logFile = fs.createWriteStream(logPath, { flags: "w" })
const logStdOut = process.stdout

// Initialize logz.io SDK
const logzio = require("logzio-nodejs").createLogger({
    token: config.keys.LOGZ_API_KEY,
    protocol: "https",
    port: "8071",
})

// Logging base function
const formatUserID = (userID: string) => {
    return userID ? userID : "No user ID"
}

export enum LogLevel {
    DEBUG = "DEBUG",
    ERROR = "ERROR",
}

const logBase = (
    title: string,
    message: string | null,
    data?: any,
    level?: LogLevel
) => {
    const debugLog = `DEBUG: ${title} -- ${message} \n ${
        data !== undefined ? JSON.stringify(data, null, 2) : ""
    }`

    logFile.write(`${util.format(debugLog)} \n`)

    if (app.isPackaged) {
        logzio.log({
            message: debugLog,
            level: level,
        })
    } else {
        console.log(debugLog)
    }
}

export const logDebug = (title: string, message: string | null, data?: any) => {
    logBase(title, message, data, LogLevel.DEBUG)
}

export const logError = (title: string, message: string | null, data?: any) => {
    logBase(title, message, data, LogLevel.ERROR)
}

export const uploadToS3 = async (
    email: any,
    accessKey = config.keys.AWS_ACCESS_KEY,
    secretKey = config.keys.AWS_SECRET_KEY,
    bucketName = "fractal-protocol-logs"
) => {
    /*
    Description:
        Uploads a local file to S3

    Arguments:
        localFilePath (string): Path of file to upload (e.g. C://log.txt)
        s3FileName (string): What to call the file once it's uploaded to S3 (e.g. "FILE.txt")
        callback (function): Callback function to fire once file is uploaded
    Returns:
        boolean: Success true/false
    */

    const s3FileName = `CLIENT_${email}_${new Date().getTime()}.txt`
    const homeDir = os.homedir()
    let localFilePath = `${homeDir}/.fractal`

    if (fs.existsSync(`${localFilePath}/log-dev.txt`)) {
        localFilePath += "/log-dev.txt"
    } else if (fs.existsSync(`${localFilePath}/log-staging.txt`)) {
        localFilePath += "/log-staging.txt"
    } else if (fs.existsSync(`${localFilePath}/log.txt`)) {
        localFilePath += "/log.txt"
    }

    console.log(`Sending log ${s3FileName} from ${localFilePath}`)

    const s3 = new AWS.S3({
        accessKeyId: accessKey,
        secretAccessKey: secretKey,
    })
    // Read file into buffer
    try {
        const fileContent = fs.readFileSync(localFilePath)
        // Set up S3 upload parameters
        const params = {
            Bucket: bucketName,
            Key: s3FileName,
            Body: fileContent,
        }
        // Upload files to the bucket
        const res = await new Promise((resolve, reject) => {
            s3.upload(params, (err: any, data: any) => {
                if (err) {
                    console.log(`Error sending log ${s3FileName}: `, err)
                    reject(err)
                } else {
                    console.log(
                        `Uploaded log ${s3FileName} from ${localFilePath}`
                    )
                    resolve(data)
                }
            })
        })

        return res
    } catch (unknownErr) {
        console.log(unknownErr)
    }
}
