import { app } from "electron"
import { of, fromEvent, merge, zip, withLatestFrom } from "rxjs"
import { filter, switchMap, take, mapTo } from "rxjs/operators"
import logRotate from "log-rotate"
import path from "path"
import fs from "fs"

import {
  electronLogPath,
  openElectronLogFile,
  formatLogs,
  filterLogData,
  logToLogzio,
  openProtocolLogFile,
} from "@app/main/utils/logging"
import { loggingBaseFilePath, loggingFiles } from "@app/config/environment"
import { fromTrigger } from "@app/main/utils/flows"
import { WhistTrigger } from "@app/constants/triggers"
import { userEmail } from "@app/main/utils/state"
import { sessionID } from "@app/constants/app"
import { appEnvironment, WhistEnvironments } from "../../../config/configs"

let electronLogFile: fs.WriteStream | undefined

app.setPath("userData", loggingBaseFilePath)

// Initialize Electron log rotation
logRotate(
  path.join(electronLogPath, loggingFiles.client),
  { count: 6 },
  (err: any) => console.error(err)
)

// Initialize protocol log rotation
logRotate(
  path.join(electronLogPath, loggingFiles.protocol),
  { count: 6 },
  (err: any) => console.error(err)
)

const sleep = of(process.argv.includes("--sleep"))
const shouldLog = zip(
  fromEvent(app, "ready"),
  merge(
    sleep.pipe(filter((sleep) => !sleep)),
    sleep.pipe(
      filter((s) => s),
      switchMap(() => fromEvent(app, "activate").pipe(take(1)))
    )
  )
).pipe(mapTo(true))
const shouldNotLog = sleep.pipe(
  filter((sleep) => sleep),
  mapTo(false)
)

/* eslint-disable @typescript-eslint/restrict-template-expressions */
fromTrigger(WhistTrigger.logging)
  .pipe(withLatestFrom(merge(shouldLog, shouldNotLog), userEmail))
  .subscribe(([logs, shouldLog, email]: [any, boolean, string]) => {
    if (shouldLog) {
      if (electronLogFile === undefined) {
        if (!fs.existsSync(electronLogPath))
          fs.mkdirSync(electronLogPath, { recursive: true })
        electronLogFile = openElectronLogFile()
      }

      const filtered = filterLogData(logs.data)
      const formatted = formatLogs(
        `${logs.title as string} -- ${email} -- ${(
          logs.msElapsed ?? 0
        ).toString()} ms since flow/trigger was started and ${(
          Date.now() - sessionID
        ).toString()} ms since app was started`,
        filtered
      )

      electronLogFile?.write(formatted)
      if (app.isPackaged) logToLogzio(formatted, "electron")
      if (!app.isPackaged) console.log(formatted)
    }
  })

// Also send protocol logs to logz.io
const stdoutBuffer = {
  buffer: "",
}

fromTrigger(WhistTrigger.protocolStdoutData).subscribe((data: string) => {
  // Combine the previous line with the current msg
  const newmsg = `${stdoutBuffer.buffer}${data}`
  // Split on newline
  const lines = newmsg.split(/\r?\n/)
  // Leave the last line in the buffer to be appended to later
  stdoutBuffer.buffer = lines.length === 0 ? "" : (lines.pop() as string)
  // Print the rest of the lines
  lines.forEach((line: string) => {
    if (!line.includes("VERBOSE")) {
      logToLogzio(line, "protocol")
      if (process.env.SHOW_PROTOCOL_LOGS === "true") console.log(line)
    }
  })
})

fromTrigger(WhistTrigger.protocolStdoutEnd).subscribe(() => {
  const { buffer } = stdoutBuffer
  if (!buffer.includes("VERBOSE")) {
    logToLogzio(buffer, "protocol")
    if (process.env.SHOW_PROTOCOL_LOGS === "true") console.log(buffer)
  }
})

/* eslint-disable @typescript-eslint/no-misused-promises */
// In non-production environments, pipe protocol logs to a local .log file
fromTrigger(WhistTrigger.protocol)
  .pipe(filter((p) => p !== undefined))
  .subscribe(async (p) => {
    if (appEnvironment !== WhistEnvironments.PRODUCTION) {
      if (!fs.existsSync(electronLogPath))
        fs.mkdirSync(electronLogPath, { recursive: true })

      const protocolLogFile = openProtocolLogFile()

      await new Promise<void>((resolve) => {
        protocolLogFile.on("open", () => resolve())
      })

      p?.stdout?.pipe(protocolLogFile)
    }
  })
