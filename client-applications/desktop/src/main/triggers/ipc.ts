/**
 * Copyright Fractal Computers, Inc. 2021
 * @file ipc.ts
 * @brief This file contains all RXJS observables created from IPC event emitters.
 */

import { ipcMain } from "electron"
import { fromEvent } from "rxjs"
import { map, share, startWith } from "rxjs/operators"
import { StateChannel } from "@app/utils/constants"
import { StateIPC } from "@app/@types/state"
import { createTrigger } from "@app/utils/flows"
import TRIGGER from "@app/utils/triggers"

// Listens for incoming messages on the single Electron IPC channel
// that our app uses to communicate with renderer processes. Messages are sent
// as a object that represents the new "state" of the system.
//
// This object will be populated with recent input from the renderer thread, and
// it might have a shape like so:
//
// {
//     userEmail: ...,
//     password: ...,
//     loginRequest: [...],
// }
//
// Downstream observables will subscribe to the individual keys of this object,
// and react when they receive new state.

// It's important that we manually startWith a value here. We need an initial
// state object to emit at the beginning of the application so downstream
// observables can initialize.

createTrigger(
    TRIGGER.eventIPC,
    fromEvent(ipcMain, StateChannel).pipe(
        map((args) => {
            if (!Array.isArray(args)) return {} as Partial<StateIPC>
            return args[1] as Partial<StateIPC>
        }),
        startWith({}),
        share()
    )
)
