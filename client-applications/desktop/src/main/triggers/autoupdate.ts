import { app } from "electron"
import { autoUpdater } from "electron-updater"
import EventEmitter from "events"
import { fromEvent, merge, of } from "rxjs"
import { takeWhile } from "rxjs/operators"

import { trigger } from "@app/utils/flows"

export const updateAvailable = trigger("updateAvailable", fromEvent(
  autoUpdater as EventEmitter,
  "update-available"
))

export const downloadProgress = trigger("downloadProgress", fromEvent(
  autoUpdater as EventEmitter,
  "download-progress"
))

export const uppdateDownloaded = trigger("updateDownloaded", fromEvent(
  autoUpdater as EventEmitter,
  "update-downloaded"
))

export const updateNotAvailable = trigger(
  "updateNotAvailable",
  merge(
    of(null).pipe(takeWhile(() => !app.isPackaged)),
    fromEvent(autoUpdater as EventEmitter, "error"),
    fromEvent(autoUpdater as EventEmitter, "update-not-available")
  )
)

