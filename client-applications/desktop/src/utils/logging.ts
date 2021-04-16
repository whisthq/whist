import { app } from 'electron'
import { tap } from 'rxjs/operators'
import { identity, truncate } from 'lodash'
import fs from 'fs'
import util from 'util'
import AWS from 'aws-sdk'
import logzio from 'logzio-nodejs'
import { merge, Observable } from 'rxjs'
import stringify from 'json-stringify-safe'

import config from '@app/utils/config'

// Logging base function
export enum LogLevel {
  DEBUG = 'DEBUG',
  WARNING = 'WARNING',
  ERROR = 'ERROR',
}

// Where to send log files, located in ~/.fractal
const getBaseFilePath = (fileName: string) => {
  const homeDir = app.getPath('home')
  const baseFilePath = `${homeDir}/.fractal`
  return `${baseFilePath}/${fileName}`
}
// Initialize logz.io SDK
const logzLogger = logzio.createLogger({
  token: config.keys.LOGZ_API_KEY,
  bufferSize: 1,
  addTimestampWithNanoSecs: true,
  sendIntervalMs: 50
})

// We use a special stringify function below before converting an object
// to JSON. This is because certain objects in our application, like HTTP
// responses and ChildProcess objects, have circular references in their
// structure. This is normal NodeJS behavior, but it can cause a runtime error
// if you blindly try to turn these objects into JSON. Our special stringify
// function strips these circular references from the object.

const logBase = (title: string, data?: any, level?: LogLevel) => {
  /*
  Description:
      Sends a log to console, debug.log file, and/or logz.io depending on if the app is packaged
  Arguments:
      title (string): Log title
      data (any): JSON or list
      level (LogLevel): Log level, see enum LogLevel above
  */
  const logPath = getBaseFilePath('debug.log')
  const logFile = fs.createWriteStream(logPath, { flags: 'a' })

  const template = `DEBUG: ${title} -- \n ${
    data !== undefined ? stringify(data, null, 2) : ''
  }`

  const debugLog = truncate(template, {
    length: 1000,
    omission: '...**logBase only prints 1000 characters per log**'
  })

  if (app.isPackaged) {
    logzLogger.log({
      message: debugLog,
      level: level
    })
  } else {
    console.log(debugLog)
  }

  logFile.write(`${util.format(debugLog)} \n`)
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

  logBase('Logs upload to S3', { s3FileName: s3FileName }, LogLevel.DEBUG)

  const uploadHelper = async (localFilePath: string) => {
    const accessKey = config.keys.AWS_ACCESS_KEY
    const secretKey = config.keys.AWS_SECRET_KEY
    const bucketName = 'fractal-protocol-logs'

    const s3 = new AWS.S3({
      accessKeyId: accessKey,
      secretAccessKey: secretKey
    })
    // Read file into buffer
    const fileContent = fs.readFileSync(localFilePath)
    // Set up S3 upload parameters
    const params = {
      Bucket: bucketName,
      Key: s3FileName,
      Body: fileContent
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
    getBaseFilePath('log-dev.txt'),
    getBaseFilePath('log-staging.txt'),
    getBaseFilePath('log.txt')
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
        message (string): Additional log message
        func (function | null | undefined): Function that transforms values emitted by the observable.
        If func is not defined, values are not modified before being logged. If null, values are not written to the log.
    Returns:
        MonoTypeOperatorFunction: logging operator
    */

  return tap<any>({
    next (value) {
      const data = identity(value)
      logBase(title, data, level)
    }
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
