import { app } from "electron"
import { autoUpdater } from "electron-updater"
import EventEmitter from "events"
import { fromEvent, merge, of } from "rxjs"
import { takeWhile } from "rxjs/operators"

import { createTrigger } from "@app/utils/flows"

// Fires if autoupdate is available
createTrigger(
  "updateAvailable",
  fromEvent(autoUpdater as EventEmitter, "update-available")
)
// Fires if autoupdate is not available, if app is not packaged, or if there's an updating error
createTrigger(
  "updateNotAvailable",
  merge(
    of(null).pipe(takeWhile(() => !app.isPackaged)),
    fromEvent(autoUpdater as EventEmitter, "error"),
    fromEvent(autoUpdater as EventEmitter, "update-not-available")
  )
)
// Fires if autoupdate is downloading, returns object with download speed, size, etc.
createTrigger(
  "downloadProgress",
  fromEvent(autoUpdater as EventEmitter, "download-progress")
)
// Fires if autoupdate is downloaded successfully
createTrigger(
  "updateDownloaded",
  fromEvent(autoUpdater as EventEmitter, "update-downloaded")
)
