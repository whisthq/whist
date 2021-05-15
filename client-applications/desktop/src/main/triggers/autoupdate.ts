import { app } from "electron"
import { autoUpdater } from "electron-updater"
import EventEmitter from "events"
import { fromEvent, merge, of } from "rxjs"
import { takeWhile } from "rxjs/operators"

import { createTrigger } from "@app/main/utils/flows"

createTrigger("updateAvailable", fromEvent(
  autoUpdater as EventEmitter,
  "update-available"
))

createTrigger("downloadProgress", fromEvent(
  autoUpdater as EventEmitter,
  "download-progress"
))

createTrigger("updateDownloaded", fromEvent(
  autoUpdater as EventEmitter,
  "update-downloaded"
))

createTrigger(
  "updateNotAvailable",
  merge(
    of(null).pipe(takeWhile(() => !app.isPackaged)),
    fromEvent(autoUpdater as EventEmitter, "error"),
    fromEvent(autoUpdater as EventEmitter, "update-not-available")
  )
)

