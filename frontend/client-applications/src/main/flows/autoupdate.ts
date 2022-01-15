import { autoUpdater } from "electron-updater"
import EventEmitter from "events"
import { fromEvent } from "rxjs"

import { flow } from "@app/main/utils/flows"

export default flow("autoUpdateFlow", () => {
  return {
    downloaded: fromEvent(autoUpdater as EventEmitter, "update-downloaded"),
    progress: fromEvent(autoUpdater as EventEmitter, "download-progress"),
  }
})
