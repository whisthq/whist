import { app } from "electron"
import { autoUpdater } from "electron-updater"
import EventEmitter from "events"
import { fromEvent, merge, of } from "rxjs"
import { takeWhile } from "rxjs/operators"

import { createTrigger } from "@app/utils/flows"
import TRIGGER from "@app/main/triggers/constants"

// Fires if autoupdate is available
createTrigger(
  TRIGGER.updateAvailable,
  fromEvent(autoUpdater as EventEmitter, "update-available")
)
// Fires if autoupdate is not available, if app is not packaged, or if there's an updating error
createTrigger(
  TRIGGER.updateNotAvailable,
  merge(
    of(null).pipe(takeWhile(() => !app.isPackaged)),
    fromEvent(autoUpdater as EventEmitter, "error"),
    fromEvent(autoUpdater as EventEmitter, "update-not-available")
  )
)
// Fires if autoupdate is downloading, returns object with download speed, size, etc.
createTrigger(
  TRIGGER.downloadProgress,
  fromEvent(autoUpdater as EventEmitter, "download-progress")
)
// Fires if autoupdate is downloaded successfully
createTrigger(
  TRIGGER.updateDownloaded,
  fromEvent(autoUpdater as EventEmitter, "update-downloaded")
)
