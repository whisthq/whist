import { app } from "electron"
import { autoUpdater } from "electron-updater"
import EventEmitter from "events"
import { fromEvent, merge, of } from "rxjs"
import { takeWhile } from "rxjs/operators"

import { createTrigger } from "@app/main/utils/flows"
import { WhistTrigger } from "@app/constants/triggers"

// Fires if autoupdate is available
createTrigger(
  WhistTrigger.updateAvailable,
  fromEvent(autoUpdater as EventEmitter, "update-available")
)
// Fires if autoupdate is not available, if app is not packaged, or if there's an updating error
createTrigger(
  WhistTrigger.updateNotAvailable,
  merge(
    of(null).pipe(takeWhile(() => !app.isPackaged)),
    fromEvent(autoUpdater as EventEmitter, "update-not-available")
  )
)
// Fires if there is an autoupdate error
createTrigger(
  WhistTrigger.updateError,
  fromEvent(autoUpdater as EventEmitter, "error")
)
// Fires when checking for update
createTrigger(
  WhistTrigger.updateChecking,
  fromEvent(autoUpdater as EventEmitter, "checking-for-update")
)
