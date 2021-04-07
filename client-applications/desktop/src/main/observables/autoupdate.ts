import { merge, from, of, fromEvent, race, interval } from "rxjs"
import {
    mapTo,
    map,
    share,
    switchMap,
    exhaustMap,
    filter,
    catchError,
} from "rxjs/operators"
import {
    eventDownloadProgress,
    eventUpdateNotAvailable,
    eventUpdateAvailable,
} from "@app/main/events/autoupdate"

// Ming's stuff

export const autoUpdateAvailable = eventUpdateAvailable.pipe(mapTo(true))

export const autoUpdateNotAvailable = eventUpdateNotAvailable.pipe(mapTo(false))

export const autoUpdateDownloadProgress =eventDownloadProgress.pipe(map(obj => JSON.stringify(obj)))

