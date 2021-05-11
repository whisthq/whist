import { app } from "electron"
import { tap } from "rxjs/operators"
import { identity, truncate } from "lodash"
import fs from "fs"
import path from "path"
import util from "util"
import AWS from "aws-sdk"
import { merge, Observable } from "rxjs"
import stringify from "json-stringify-safe"
import * as Amplitude from "@amplitude/node"
import winston from "winston"

import config, { loggingBaseFilePath } from "@app/config/environment"

const logger = winston.createLogger({
  level: "info",
  transports: [
    new winston.transports.Console(),
    new winston.transports.File({ filename: "electron.log" }),
  ],
})
const amplitude = Amplitude.init("f4009f8b53b3060953437879d9769e8b")
const sessionID = new Date().getTime()

// Log levels
export enum LogLevel {
  DEBUG = "DEBUG",
  WARNING = "WARNING",
  ERROR = "ERROR",
}

const formatLogs = (title: string, data: object) => {
  /*

  */

  // We use a special stringify function below before converting an object
  // to JSON. This is because certain objects in our application, like HTTP
  // responses and ChildProcess objects, have circular references in their
  // structure. This is normal NodeJS behavior, but it can cause a runtime error
  // if you blindly try to turn these objects into JSON. Our special stringify
  // function strips these circular references from the object.
  const template = `DEBUG: ${title} -- ${sessionID.toString()} -- \n ${
    data !== undefined ? stringify(data, null, 2) : ""
  }`

  const debugLog = truncate(template, {
    length: 1000,
    omission: "...**logBase only prints 1000 characters per log**",
  })

  return `${util.format(debugLog)} \n`
}

const winstonLog = (title: string, data: object, level?: LogLevel) => {
  const logs = formatLogs(title, data)

  if (level === LogLevel.ERROR) {
    logger.error(logs)
  } else if (level === LogLevel.WARNING) {
    logger.warning(logs)
  } else {
    logger.info(logs)
  }
}

const amplitudeLog = async (title: string, data: object, level?: LogLevel) => {
  await amplitude.logEvent({
    event_type: title,
    session_id: sessionID,
    user_id: "ming+3@fractal.co",
    event_properties: data,
  })
}

const logBase = async (title: string, data: object, level?: LogLevel) => {
  /*
  Description:
      Sends a log to console, debug.log file, and/or logz.io depending on if the app is packaged
  Arguments:
      title (string): Log title
      data (any): JSON or list
      level (LogLevel): Log level, see enum LogLevel above
  */

  if (app.isPackaged) await amplitudeLog(title, data, level)
  winstonLog(title, data, level)
}

export const uploadToS3 = async (email: string) => {
  /*
  Description:
      Uploads a local file to S3
  Arguments:
      email (string): user email of the logged in user
  Returns:
      response from the s3 upload
  */
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

export const logObservable = (level: LogLevel, title: string) => {
  /*
    Description:
        Returns a custom operator that logs values emitted by an observable
    Arguments:
        level (LogLevel): Level of log
        title (string): Name of observable, written to log
    Returns:
        MonoTypeOperatorFunction: logging operator
    */

  return tap(async (data: object) => {
    await logBase(title, data, level)
  })
}

// Log level wrapper functions
const debug = logObservable.bind(null, LogLevel.DEBUG)
const warning = logObservable.bind(null, LogLevel.WARNING)
const error = logObservable.bind(null, LogLevel.ERROR)

const logObservables = (
  func: typeof debug,
  ...args: Array<[Observable<any>, string]>
) => merge(...args.map(([obs, title]) => obs.pipe(func(title)))).subscribe()

export const debugObservables = logObservables.bind(null, debug)
export const warningObservables = logObservables.bind(null, warning)
export const errorObservables = logObservables.bind(null, error)
