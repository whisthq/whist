import { autoUpdater } from "electron-updater"
import EventEmitter from "events"
import { fromEvent } from "rxjs"

import { createTrigger, flow } from "@app/utils/flows"
import TRIGGER from "@app/utils/triggers"

export default flow("autoUpdateFlow", () => {
  return {
    downloaded: createTrigger(
      TRIGGER.updateDownloaded,
      fromEvent(autoUpdater as EventEmitter, "update-downloaded")
    ),
    progress: createTrigger(
      TRIGGER.downloadProgress,
      fromEvent(autoUpdater as EventEmitter, "download-progress")
    ),
  }
})
