/**
 * Copyright Fractal Computers, Inc. 2021
 * @file ipc.ts
 * @brief This file contains all RXJS observables created from IPC event emitters.
 */

import { ipcMain } from "electron"
import { fromEvent } from "rxjs"
import { get } from "lodash"
import { map, share } from "rxjs/operators"
import { StateChannel } from "@app/utils/constants"
import { StateIPC } from "@app/utils/types"

export const eventIPC = fromEvent(ipcMain, StateChannel).pipe(
    map(([_event, state]) => state as Partial<StateIPC>),
    share()
)

export const fromEventIPC = (...keys: Array<keyof StateIPC>) =>
    eventIPC.pipe(
        map((obj) => get(obj as Partial<StateIPC>, keys)),
        share()
    )
