import { mapTo, map } from "rxjs/operators"
import {
  eventDownloadProgress,
  eventUpdateNotAvailable,
  eventUpdateAvailable,
} from "@app/main/events/autoupdate"

export const autoUpdateAvailable = eventUpdateAvailable.pipe(mapTo(true))

export const autoUpdateNotAvailable = eventUpdateNotAvailable.pipe(mapTo(false))

export const autoUpdateDownloadProgress = eventDownloadProgress.pipe(
  map((obj) => JSON.stringify(obj))
)
