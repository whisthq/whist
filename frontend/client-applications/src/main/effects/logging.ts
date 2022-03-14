import { app } from "electron"
import { of, fromEvent, merge, zip, withLatestFrom } from "rxjs"
import { filter, switchMap, take, mapTo } from "rxjs/operators"
import logRotate from "log-rotate"
import path from "path"
import fs from "fs"

import {
  electronLogPath,
  openLogFile,
  formatLogs,
  filterLogData,
  logToLogzio,
} from "@app/main/utils/logging"
import { loggingBaseFilePath, loggingFiles } from "@app/config/environment"
import { fromTrigger } from "@app/main/utils/flows"
import { WhistTrigger } from "@app/constants/triggers"
import { userEmail } from "@app/main/utils/state"
import { sessionID } from "@app/constants/app"

let logFile: fs.WriteStream | undefined

app.setPath("userData", loggingBaseFilePath)

// Initialize Electron log rotation
logRotate(
  path.join(electronLogPath, loggingFiles.client),
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
      if (logFile === undefined) logFile = openLogFile()

      const filtered = filterLogData(logs.data)
      const formatted = formatLogs(
        `${logs.title as string} -- ${email} -- ${(
          logs.msElapsed ?? 0
        ).toString()} ms since flow/trigger was started and ${(
          Date.now() - sessionID
        ).toString()} ms since app was started`,
        filtered
      )

      logFile?.write(formatted)
      if (app.isPackaged) logToLogzio(formatted, "electron")
      if (!app.isPackaged) console.log(formatted)
    }
  })
