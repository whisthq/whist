import { app } from 'electron'
import { tap } from 'rxjs/operators'
import { identity, truncate } from 'lodash'
import fs from 'fs'
import os from 'os'
import util from 'util'
import path from 'path'
import AWS from 'aws-sdk'
import logzio from 'logzio-nodejs'
import stringify from 'json-stringify-safe'

import config from '@app/utils/config'

// Where to send .log file
const logPath = path.join(
  app.getAppPath().replace('/Resources/app.asar', ''),
  'debug.log'
)
const logFile = fs.createWriteStream(logPath, { flags: 'a' })

// Initialize logz.io SDK
const logzLogger = logzio.createLogger({
  token: config.keys.LOGZ_API_KEY,
  bufferSize: 1,
  addTimestampWithNanoSecs: true,
  sendIntervalMs: 50
})

// Logging base function
export enum LogLevel {
  DEBUG = 'DEBUG',
  INFO = 'INFO',
  WARNING = 'WARNING',
  ERROR = 'ERROR',
}

const logBase = (
  title: string,
  message: string | null,
  data?: any,
  level?: LogLevel
) => {
  const template = `DEBUG: ${title} -- ${message ?? ''} \n ${
    data !== undefined ? stringify(data, null, 2) : ''
  }`

  const debugLog = truncate(template, { length: 1000, omission: '...(logBase only prints 1000 characters per log)' })

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

export const logDebug = (title: string, message: string | null, data?: any) => {
  logBase(title, message, data, LogLevel.DEBUG)
}

export const logWarning = (title: string, message: string | null, data?: any) => {
  logBase(title, message, data, LogLevel.WARNING)
}

export const logError = (title: string, message: string | null, data?: any) => {
  logBase(title, message, data, LogLevel.ERROR)
}

export const uploadToS3 = async (
  email: string
) => {
  /*
  Description:
      Uploads a local file to S3
  Arguments:
      email (string): user email of the logged in user
  Returns:
      response from the s3 upload
  */
  const s3FileName = `CLIENT_${email}_${new Date().getTime()}.txt`
  logDebug('value: ', 'Protocol logs sent to', { fileName: s3FileName })

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

  const homeDir = os.homedir()
  const baseFilePath = `${homeDir}/.fractal`
  const uploadPromises: Array<Promise<any>> = []

  const logLocations = [
    `${baseFilePath}/log-dev.txt`,
    `${baseFilePath}/log-staging.txt`,
    `${baseFilePath}/log.txt`
  ]

  logLocations.forEach((filePath: string) => {
    if (fs.existsSync(filePath)) {
      uploadPromises.push(uploadHelper(filePath))
    }
  })

  await Promise.all(uploadPromises)
}

export const log = (
  level: LogLevel,
  title: string,
  message: string = 'value:',
  func?: null | ((...args: any[]) => any)
) => {
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
      if (func === undefined) func = identity
      const data = func != null ? func(value) : undefined
      logBase(title, message, data, level)
    }
  })
}

// Log level wrapper functions
export const debug = log.bind(null, LogLevel.DEBUG)
export const info = log.bind(null, LogLevel.INFO)
export const warning = log.bind(null, LogLevel.WARNING)
export const error = log.bind(null, LogLevel.ERROR)
