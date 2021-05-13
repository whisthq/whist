import { app } from "electron"
import { autoUpdater } from "electron-updater"
import EventEmitter from "events"
import { fromEvent, merge, of } from "rxjs"
import { takeWhile } from "rxjs/operators"

import { trigger } from "@app/main/utils/flows"

trigger("updateAvailable", fromEvent(
  autoUpdater as EventEmitter,
  "update-available"
))

trigger("downloadProgress", fromEvent(
  autoUpdater as EventEmitter,
  "download-progress"
))

trigger("updateDownloaded", fromEvent(
  autoUpdater as EventEmitter,
  "update-downloaded"
))

trigger(
  "updateNotAvailable",
  merge(
    of(null).pipe(takeWhile(() => !app.isPackaged)),
    fromEvent(autoUpdater as EventEmitter, "error"),
    fromEvent(autoUpdater as EventEmitter, "update-not-available")
  )
)

