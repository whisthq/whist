/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file logging.ts
 * @brief This file contains utility functions for logging.
 */

import fs from "fs"
import path from "path"
import util from "util"
import * as Amplitude from "@amplitude/node"
import cloneDeep from "lodash.clonedeep"
import mapValuesDeep from "deepdash/mapValuesDeep"

import config, {
  loggingBaseFilePath,
  loggingFiles,
} from "@app/config/environment"
import { persistGet } from "@app/main/utils/persist"
import { sessionID } from "@app/constants/app"
import { createLogger } from "logzio-nodejs"
import { CACHED_USER_EMAIL } from "@app/constants/store"

const amplitude = Amplitude.init(config.keys.AMPLITUDE_KEY)
const electronLogPath = path.join(loggingBaseFilePath, "logs")
const logzio = createLogger({
  token: config.keys.LOGZ_KEY,
})
// Open a file handle to append to the logs file.
// Create the loggingBaseFilePath directory if it does not exist.
const openLogFile = () => {
  fs.mkdirSync(electronLogPath, { recursive: true })
  const logPath = path.join(electronLogPath, loggingFiles.client)
  return fs.createWriteStream(logPath)
}

const formatLogs = (title: string, data: object) => {
  // We use a special stringify function below before converting an object
  // to JSON. This is because certain objects in our application, like HTTP
  // responses and ChildProcess objects, have circular references in their
  // structure. This is normal NodeJS behavior, but it can cause a runtime error
  // if you blindly try to turn these objects into JSON. Our special stringify
  // function strips these circular references from the object.
  const template = `${title} -- ${sessionID.toString()} -- \n ${util.inspect(
    data
  )}`

  return `${util.format(template)} \n`
}

const filterLogData = (data: object) => {
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
        "localStorage",
      ].includes(k) &&
      (v ?? undefined) !== undefined
    ) {
      const stringified = JSON.stringify(v)
      return `${stringified.slice(
        0,
        Math.min(5, stringified.length)
      )} **********`
    }
    return v
  })

  return dataClone
}

const logToAmplitude = async (title: string, data?: object) => {
  const userEmail = persistGet(CACHED_USER_EMAIL) ?? ""

  if (userEmail !== "")
    await amplitude.logEvent({
      event_type: `[${(config.appEnvironment as string) ?? "LOCAL"}] ${title}`,
      session_id: sessionID,
      user_id: userEmail as string,
      event_properties: data ?? {},
    })
}

const logToLogzio = (line: string, subComponent: string) => {
  // This function will push to logz.io on each log line received from the protocol

  // If a line starts with {, then we check if it is in JSON format. If yes, we post
  // the JSON data directly to logz.io. Useful for reporting metrics to ELK stack.
  if (line.startsWith("{ ")) {
    try {
      const output = JSON.parse(line)
      output.level = "METRIC"
      output.session_id = sessionID
      output.component = "clientapp"
      output.sub_component = subComponent
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
    sub_component: subComponent,
  })
}

export {
  electronLogPath,
  openLogFile,
  formatLogs,
  filterLogData,
  logToAmplitude,
  logToLogzio,
}
