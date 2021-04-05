import { app } from "electron"
import fs from "fs"
import util from "util"
import path from "path"

import config from "@app/utils/config"

// Where to send .log file
const logPath = path.join(
    app.getAppPath().replace("/Resources/app.asar", ""),
    "debug.log"
)
const logFile = fs.createWriteStream(logPath, { flags: "w" })
const logStdOut = process.stdout

// Initialize logz.io SDK
const logzio = require("logzio-nodejs").createLogger({
    token: config.keys.LOGZ_API_KEY,
    protocol: "https",
    port: "8071",
})

// Logging base function
const formatUserID = (userID: string) => {
    return userID ? userID : "No user ID"
}

enum LogLevel {
    DEBUG = "DEBUG",
    ERROR = "ERROR",
}

const logBase = (logs: string, userID = "", level = LogLevel.DEBUG) => {
    const formattedLogs = `${formatUserID(userID)} | ${logs}`
    logFile.write(util.format(formattedLogs) + "\n")
    logStdOut.write(util.format(formattedLogs) + "\n")

    if (app.isPackaged)
        logzio.log({
            message: logs,
            userID: userID,
            level: level,
        })
}

export const logDebug = (title: string, message: string | null, data?: any) => {
    const debugLog = `DEBUG: ${title} -- ${message} \n ${(data !== undefined ? JSON.stringify(data, null, 2) : "")}`

    logFile.write(`${util.format(debugLog)} \n`)

    if (app.isPackaged) {
        logzio.log({
            message: debugLog,
            level: LogLevel.DEBUG
        })
    } else {
        console.log(debugLog)
    }
}

// Export logging functions
export const logInfo = (logs: string, userID = "") => {
    logBase(logs, userID, LogLevel.DEBUG)
}

export const logError = (logs: string, userID = "") => {
    logBase(logs, userID, LogLevel.ERROR)
}
