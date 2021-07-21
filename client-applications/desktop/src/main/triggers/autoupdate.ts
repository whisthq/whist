import { app } from "electron"
import { autoUpdater } from "electron-updater"
import EventEmitter from "events"
import { fromEvent, merge, of } from "rxjs"
import { takeWhile } from "rxjs/operators"

import { createTrigger } from "@app/utils/flows"
import TRIGGER from "@app/utils/triggers"

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
        fromEvent(autoUpdater as EventEmitter, "update-not-available")
    )
)
// Fires if there is an autoupdate error
createTrigger(
    TRIGGER.updateError,
    fromEvent(autoUpdater as EventEmitter, "error")
)
// Fires when checking for update
createTrigger(
    TRIGGER.updateChecking,
    fromEvent(autoUpdater as EventEmitter, "checking-for-update")
)
