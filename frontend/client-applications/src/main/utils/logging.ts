/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file logging.ts
 * @brief This file contains utility functions for logging.
 */

import { app } from "electron"
import { fromEvent, zip, merge, of } from "rxjs"
import { filter, switchMap, take, map, startWith } from "rxjs/operators"
import fs from "fs"
import path from "path"
import util from "util"
import * as Amplitude from "@amplitude/node"
import cloneDeep from "lodash.clonedeep"
import mapValuesDeep from "deepdash/mapValuesDeep"
import logRotate from "log-rotate"

import config, {
  loggingBaseFilePath,
  loggingFiles,
} from "@app/config/environment"
import { persistGet } from "@app/main/utils/persist"
import { sessionID } from "@app/constants/app"
import { createLogger } from "logzio-nodejs"
import { CACHED_USER_EMAIL } from "@app/constants/store"

app.setPath("userData", loggingBaseFilePath)

const amplitude = Amplitude.init(config.keys.AMPLITUDE_KEY)
export const electronLogPath = path.join(loggingBaseFilePath, "logs")

const sleep = of(process.argv.includes("--sleep"))

export const shouldStoreLogs = zip(
  fromEvent(app, "ready"),
  merge(
    sleep.pipe(filter((sleep) => !sleep)),
    sleep.pipe(
      filter((s) => s),
      switchMap(() => fromEvent(app, "activate").pipe(take(1)))
    )
  )
).pipe(
  map(() => app.isPackaged),
  filter((shouldLog) => shouldLog),
  startWith(false)
)

// Initialize protocol log rotation
logRotate(
  path.join(electronLogPath, loggingFiles.protocol),
  { count: 4 },
  (err: any) => console.error(err)
)

// Initialize Electron log rotation
logRotate(
  path.join(electronLogPath, loggingFiles.client),
  { count: 4 },
  (err: any) => console.error(err)
)

// Open a file handle to append to the logs file.
// Create the loggingBaseFilePath directory if it does not exist.
const openLogFile = () => {
  fs.mkdirSync(electronLogPath, { recursive: true })
  const logPath = path.join(electronLogPath, loggingFiles.client)
  return fs.createWriteStream(logPath)
}

const logzio = createLogger({
  token: config.keys.LOGZ_KEY,
})

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

  return `${util.format(template)} \n`
}

const amplitudeLog = (
  title: string,
  data: object,
  userEmail: string,
  msElapsed?: number
) => {
  if (userEmail !== "")
    amplitude
      .logEvent({
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
      .catch((err) => console.error(err))
}

export const logging = (
  title: string,
  data: object,
  level?: LogLevel,
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

  // Don't log the config token to Amplitude to protect user privacy
  let dataClone = cloneDeep(data)
  dataClone = mapValuesDeep(dataClone, (v: object | any[], k: string) => {
    if (
      [
        "configToken",
        "config_encryption_token",
        "cookies",
        "extensions",
        "importedData",
        "bookmarks",
        "urls",
      ].includes(k) &&
      (v ?? undefined) !== undefined
    ) {
      return "***********"
    }
    return v
  })

  const userEmail = persistGet(CACHED_USER_EMAIL) ?? ""

  const logs = formatLogs(
    `${title} -- ${userEmail as string} -- ${(msElapsed !== undefined
      ? msElapsed
      : 0
    ).toString()} ms since flow/trigger was started and ${(
      Date.now() - sessionID
    ).toString()} ms since app was started`,
    dataClone,
    LogLevel.DEBUG
  )

  shouldStoreLogs.subscribe((shouldLog: boolean) => {
    if (shouldLog) {
      logFile.write(logs)
      amplitudeLog(title, dataClone, userEmail as string, msElapsed)
    }
  })

  if (!app.isPackaged) console.log(logs)
}

export const protocolToLogz = (line: string) => {
  // This function will push to logz.io on each log line received from the protocol

  // If a line starts with {, then we check if it is in JSON format. If yes, we post
  // the JSON data directly to logz.io. Useful for reporting metrics to ELK stack.
  if (line.startsWith("{ ")) {
    try {
      const output = JSON.parse(line)
      output.level = "METRIC"
      output.session_id = sessionID
      output.component = "clientapp"
      logzio.log(output)
      return
    } catch (err) {
      // Let it follow a normal flow in case of any JSON.parse error
    }
  }
  const match = line.match(/^[\d:.]*\s*\|\s*(?<level>\w+)\s*\|/)
  const level = match?.groups?.level ?? "INFO"

  logzio.log({
    level: level,
    message: line,
    session_id: sessionID,
    component: "clientapp",
  })
}
