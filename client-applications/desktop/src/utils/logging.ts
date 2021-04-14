import { app } from 'electron'
import fs from 'fs'
import os from 'os'
import util from 'util'
import path from 'path'
import AWS from 'aws-sdk'
import logzio from 'logzio-nodejs'

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
  protocol: 'https',
  port: '8071'
})

// Logging base function
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
  const debugLog = `DEBUG: ${title} -- ${message ?? ''} \n ${
    data !== undefined ? JSON.stringify(data, null, 2) : ''
  }`

  logFile.write(`${util.format(debugLog)} \n`)

  if (app.isPackaged) {
    logzLogger.log({
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

export const uploadToS3 = async (
  email: string,
  accessKey = config.keys.AWS_ACCESS_KEY,
  secretKey = config.keys.AWS_SECRET_KEY,
  bucketName = 'fractal-protocol-logs'
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
  const homeDir = os.homedir()
  let localFilePath = `${homeDir}/.fractal`

  if (fs.existsSync(`${localFilePath}/log-dev.txt`)) {
    localFilePath += '/log-dev.txt'
  } else if (fs.existsSync(`${localFilePath}/log-staging.txt`)) {
    localFilePath += '/log-staging.txt'
  } else if (fs.existsSync(`${localFilePath}/log.txt`)) {
    localFilePath += '/log.txt'
  }

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
    await new Promise((resolve, reject) => {
      s3.upload(params, (err: any | null, data: any) => {
        if (err !== null) {
          logDebug("error: ", `${err.toString()}`)
          reject(err)
        } else {
          resolve(data)
        }
      })
    })

    logDebug("value: ", `Uploaded to ${s3FileName}`)

    return
  } catch (unknownErr) {
    console.log(unknownErr)
  }
}
