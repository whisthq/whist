import { app } from "electron"
import { tap } from "rxjs/operators"
import { truncate } from "lodash"
import fs from "fs"
import path from "path"
import util from "util"
import AWS from "aws-sdk"
import { merge, Observable, zip, of } from "rxjs"
import stringify from "json-stringify-safe"
import * as Amplitude from "@amplitude/node"

import config, { loggingBaseFilePath } from "@app/config/environment"

const amplitude = Amplitude.init(config.keys.AMPLITUDE_KEY)
const sessionID = new Date().getTime()

// Open a file handle to append to the logs file.
// Create the loggingBaseFilePath directory if it does not exist.
const openLogFile = () => {
  fs.mkdirSync(loggingBaseFilePath, { recursive: true })
  const logPath = path.join(loggingBaseFilePath, "debug.log")
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
  const template = `${level}: ${title} -- ${sessionID.toString()} -- \n ${
    data !== undefined ? stringify(data, null, 2) : ""
  }`

  const debugLog = truncate(template, {
    length: 1000,
    omission: "...**logBase only prints 1000 characters per log**",
  })

  return `${util.format(debugLog)} \n`
}

const localLog = (title: string, data: object, level: LogLevel, userID: string) => {
  const logs = formatLogs(`${title} -- ${userID}`, data, level)

  if (!app.isPackaged) console.log(logs)

  logFile.write(logs)
}

const amplitudeLog = async (
  title: string,
  data: object,
  userID: string | undefined
) => {
  if (userID !== undefined) {
    await amplitude.logEvent({
      event_type: `[${(config.appEnvironment as string) ?? "LOCAL"}] ${title}`,
      session_id: sessionID,
      user_id: userID,
      event_properties: data,
    })
  }
}

export const logBase = async (
  title: string,
  data: object,
  level: LogLevel,
  userID?: string
) => {
  /*
  Description:
      Sends a log to console, debug.log file, and/or logz.io depending on if the app is packaged
  Arguments:
      title (string): Log title
      data (any): JSON or list
      level (LogLevel): Log level, see enum LogLevel above
  */

  await amplitudeLog(title, data, userID)
  localLog(title, data, level, userID)
}

export const uploadToS3 = async (email: string) => {
  /*
  Description:
      Uploads a local file to S3
  Arguments:
      email (string): user email of the logged in user
  Returns:
      Response from the s3 upload
  */
  const s3FileName = `CLIENT_${email}_${new Date().getTime()}.txt`

  await logBase(
    "Logs upload to S3",
    { s3FileName: s3FileName },
    LogLevel.DEBUG,
    email
  )

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

  const uploadPromises: Array<Promise<any>> = []

  const logLocations = [
    path.join(loggingBaseFilePath, "log-dev.txt"),
    path.join(loggingBaseFilePath, "log-staging.txt"),
    path.join(loggingBaseFilePath, "log.txt"),
  ]

  logLocations.forEach((filePath: string) => {
    if (fs.existsSync(filePath)) {
      uploadPromises.push(uploadHelper(filePath))
    }
  })

  await Promise.all(uploadPromises)
}


