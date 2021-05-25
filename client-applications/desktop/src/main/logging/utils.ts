import { app } from "electron"
import { inspect } from "util"
import { truncate, isEmpty, split, includes } from "lodash"
import chalk from "chalk"
import fs from "fs"
import path from "path"
import * as Amplitude from "@amplitude/node"
import config, { loggingBaseFilePath } from "@app/config/environment"

// Some types we'll use throughout the logging folder.
//
// LoggingFormat describes the tuple of [message, transformFn] that makes
// up the values of the LoggingSchema maps.
export type LoggingFormat = [string, (val: any) => any]
// LoggingSchema describes the data structures in the logging/schema
// folder. These contain information about how to format the logging
// output of a flow.
// A "null" key indicates "do not log this flow".
export type LoggingSchema = {
  [name: string]: {
    [key: string]: LoggingFormat | null
  }
}

// LoggingSink describes a function which actually performs the logging.
// We'll let them all have the same signature. The first argument "context"
// is a map that can contain some current program state, like the email
// of the logged in user.
export type LoggingSink = (
  context: { [key: string]: any },
  title: string,
  message: string,
  data: any
) => void

// Setting up logging to console output.
// The popular "chalk" library can provide some color to our logs.
const chalkTitle = (s: string) => {
  const parts = split(s, ".")
  const base = chalk.bold
  if (includes(parts, "success")) return base.green(s)
  if (includes(parts, "failure")) return base.red(s)
  return base.yellow(s)
}

const chalkMessage = (s: string) => chalk.cyan(s)

export const logFormat = (title: string, message: string, value: any) => {
  value = truncate(inspect(value), {
    length: 1000,
    omission: "...**only printing 1000 characters per log**",
  })

  if (isEmpty(message)) return `${chalkTitle(title)} -- ${value}`
  return `${chalkTitle(title)} -- ${chalkMessage(message)} -- ${value}`
}

export const consoleLog: LoggingSink = (_context, title, message, data) => {
  if (!app.isPackaged) console.log(logFormat(title, message, data))
}

// Setting up logging to amplitude.

// Some one-time amplitude setup. The sessionID will just be when
// the user started the program.
const amplitude = Amplitude.init(config.keys.AMPLITUDE_KEY)
const sessionID = new Date().getTime()

export const amplitudeLog: LoggingSink = (context, title, _message, data) => {
  if (context.email && context.email !== "") {
    amplitude.logEvent({
      event_type: `[${(config.appEnvironment as string) ?? "LOCAL"}] ${title}`,
      session_id: sessionID,
      user_id: context.email,
      event_properties: data,
    })
  }
}

// Setting up logging to local files.
// This local logger will also handle uploading to s3 if the app is
// closing.

const openLogFile = (filePath: string) => {
  fs.mkdirSync(filePath, { recursive: true })
  return fs.createWriteStream(path.join(filePath, "debug.log"))
}

const logFile = openLogFile(loggingBaseFilePath)

export const fileLog: LoggingSink = (_context, title, message, data) => {
  logFile.write(logFormat(title, message, data))
}
