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

import config, {
  getLoggingBaseFilePath,
  loggingFiles,
} from "@app/config/environment"
import { persistGet } from "@app/utils/persist"

const amplitude = Amplitude.init(config.keys.AMPLITUDE_KEY)
const sessionID = new Date().getTime()

// Open a file handle to append to the logs file.
// Create the loggingBaseFilePath directory if it does not exist.
const openLogFile = () => {
  const loggingBaseFilePath = getLoggingBaseFilePath()
  fs.mkdirSync(loggingBaseFilePath, { recursive: true })
  const logPath = path.join(loggingBaseFilePath, loggingFiles.client)
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
  email: string
) => {
  const logs = formatLogs(`${title} -- ${email}`, data, level)

  if (!app.isPackaged) console.log(logs)

  logFile.write(logs)
}

const amplitudeLog = async (title: string, data: object, email: string) => {
  if (email !== "") {
    await amplitude.logEvent({
      event_type: `[${(config.appEnvironment as string) ?? "LOCAL"}] ${title}`,
      session_id: sessionID,
      user_id: email,
      event_properties: { data: util.inspect(data) },
    })
  }
}

export const logBase = async (title: string, data: object, level: LogLevel) => {
  /*
  Description:
      Sends a log to console, client.log file, and/or logz.io depending on if the app is packaged
  Arguments:
      title (string): Log title
      data (any): JSON or list
      level (LogLevel): Log level, see enum LogLevel above
  */
  const email = persistGet("email") ?? ""

  await amplitudeLog(title, data, email)
  localLog(title, data, level, email)
}

export const uploadToS3 = async () => {
  /*
  Description:
      Uploads a local file to S3
  Arguments:
      email (string): user email of the logged in user
  Returns:
      Response from the s3 upload
  */
  const email = persistGet("email") ?? ""

  if (email === "") return

  const s3FileName = `CLIENT_${email}_${new Date().getTime()}.txt`

  await logBase("Logs upload to S3", { s3FileName: s3FileName }, LogLevel.DEBUG)

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

  const logLocation = path.join(getLoggingBaseFilePath(), loggingFiles.protocol)

  if (fs.existsSync(logLocation)) {
    await uploadHelper(logLocation)
  }
}
