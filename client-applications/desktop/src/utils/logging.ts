import { app } from 'electron'
import fs from 'fs'
import util from 'util'
import path from 'path'
import AWS from 'aws-sdk'

import config from '@app/utils/config'

// Where to send .log file
const logPath = path.join(
  app.getAppPath().replace('/Resources/app.asar', ''),
  'debug.log'
)
const logFile = fs.createWriteStream(logPath, { flags: 'w' })
const logStdOut = process.stdout

// Initialize logz.io SDK
const logzio = require('logzio-nodejs').createLogger({
  token: config.keys.LOGZ_API_KEY,
  protocol: 'https',
  port: '8071'
})

// Logging base function
const formatUserID = (userID: string) => {
  return userID || 'No user ID'
}

export enum LogLevel {
  DEBUG = 'DEBUG',
  ERROR = 'ERROR',
}

const logBase = (
  title: string,
  message: string | null,
  data?: any,
  level?: LogLevel
) => {
  const debugLog = `DEBUG: ${title} -- ${message} \n ${
        data !== undefined ? JSON.stringify(data, null, 2) : ''
    }`

  logFile.write(`${util.format(debugLog)} \n`)

  if (app.isPackaged) {
    logzio.log({
      message: debugLog,
      level: level
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

export const uploadToS3 = (
  localFilePath: string,
  s3FileName: string,
  accessKey = config.keys.AWS_ACCESS_KEY,
  secretKey = config.keys.AWS_SECRET_KEY,
  bucketName = 'fractal-protocol-logs'
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

  const s3 = new AWS.S3({
    accessKeyId: accessKey,
    secretAccessKey: secretKey
  })
  // Read file into buffer
  try {
    const fileContent = fs.readFileSync(localFilePath)

    // Set up S3 upload parameters
    const params = {
      Bucket: bucketName,
      Key: s3FileName,
      Body: fileContent
    }

    // Upload files to the bucket
    s3.upload(params, (s3Error: any) => {
      return s3Error
    })
  } catch (unknownErr) {
    return unknownErr
  }
}
