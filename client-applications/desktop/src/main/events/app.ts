/**
 * Copyright Fractal Computers, Inc. 2021
 * @file app.ts
 * @brief This file contains all RXJS observables created from Electron app event emitters.
 */

import { app } from 'electron'
import EventEmitter from 'events'
import { fromEvent } from 'rxjs'

export const eventAppReady = fromEvent(app as EventEmitter, 'ready')

export const eventAppActivation = fromEvent(app as EventEmitter, 'activate')

export const eventWindowsAllClosed = fromEvent(
  app as EventEmitter,
  'window-all-closed'
)

// By default, the window-all-closed Electron event will cause the application
// to close. We don't want this behavior, we want to control for ourselves
// when the application closes with observables and Effects.
// We subscribe to window-all-closed here and call preventDefault().
// The event will still emit, but the app won't quit automatically.
app.on('window-all-closed', (event: any) => event.preventDefault())
