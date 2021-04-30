import { app } from "electron"
import { autoUpdater } from "electron-updater"
import EventEmitter from "events"
import { fromEvent, merge, of } from "rxjs"
import { takeWhile } from "rxjs/operators"

import { debugObservables } from "@app/utils/logging"

export const eventUpdateAvailable = fromEvent(
  autoUpdater as EventEmitter,
  "update-available"
)

export const eventDownloadProgress = fromEvent(
  autoUpdater as EventEmitter,
  "download-progress"
)

export const eventUpdateDownloaded = fromEvent(
  autoUpdater as EventEmitter,
  "update-downloaded"
)

export const eventUpdateError = fromEvent(autoUpdater as EventEmitter, "error")

export const eventUpdateNotAvailable = merge(
  of(null).pipe(takeWhile(() => !app.isPackaged)),
  eventUpdateError,
  fromEvent(autoUpdater as EventEmitter, "update-not-available")
)

debugObservables(
  [eventUpdateAvailable, "eventUpdateAvailable"],
  [eventUpdateNotAvailable, "eventUpdateNotAvailable"],
  [eventDownloadProgress, "eventDownloadProgress"],
  [eventUpdateDownloaded, "eventUpdateDownloaded"],
  [eventUpdateError, "eventUpdateError"]
)
