/**
 * Copyright Fractal Computers, Inc. 2021
 * @file ipc.ts
 * @brief This file contains all RXJS observables created from IPC event emitters.
 */

import { ipcMain } from 'electron'
import { fromEvent } from 'rxjs'
import { get } from 'lodash'
import { map, share, startWith } from 'rxjs/operators'
import { StateChannel } from '@app/utils/constants'
import { StateIPC } from '@app/utils/types'
import { LogLevel, debug } from '@app/utils/logging'

// This file listens for incoming messages on the single Electron IPC channel
// that our app uses to communicate with renderer processes. Messages are sent
// as a object that represents the new "state" of the system.
//
// This object will be populated with recent input from the renderer thread, and
// it might have a shape like so:
//
// {
//     email: ...,
//     password: ...,
//     loginRequest: [...],
// }
//
// Downstream observables will subscribe to the individual keys of this object,
// and react when they receive new state.

// Once again, rxjs and Typescript do not agree about type inference, so we
// manually coerce.

// It's important that we manually startWith a value here. We need an initial
// state object to emit at the beginning of the application so downstream
// observables can initialize.

export const eventIPC = fromEvent(ipcMain, StateChannel).pipe(
  map(([_event, state]) => state as Partial<StateIPC>),
  startWith({}),
  share(),
  debug(LogLevel.DEBUG, 'eventIPC')
)

export const fromEventIPC = (...keys: Array<keyof StateIPC>) =>
  eventIPC.pipe(
    map((obj) => get(obj as Partial<StateIPC>, keys)),
    share()
  )
