import { autoUpdater } from 'electron-updater'
import EventEmitter from 'events'
import { fromEvent } from 'rxjs'

import { logDebug } from '@app/utils/logging'

export const eventUpdateAvailable = fromEvent(
  autoUpdater as EventEmitter,
  'update-available'
)

export const eventUpdateNotAvailable = fromEvent(
  autoUpdater as EventEmitter,
  'update-not-available'
)

export const eventDownloadProgress = fromEvent(
  autoUpdater as EventEmitter,
  'download-progress'
)

export const eventUpdateDownloaded = fromEvent(
  autoUpdater as EventEmitter,
  'update-downloaded'
)
