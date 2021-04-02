/**
 * Copyright Fractal Computers, Inc. 2021
 * @file ipc.ts
 * @brief This file contains all RXJS observables created from IPC event emitters.
*/

import { ipcMain } from "electron"
import { fromEvent } from "rxjs"
import { map, share, pluck } from "rxjs/operators"
import { StateChannel } from "@app/utils/constants"

export const eventIPC = fromEvent(ipcMain, StateChannel).pipe(
    map(([_event, state]) => state),
    share()
)

export const fromEventIPC = (...keys: string[]) =>
    eventIPC.pipe(pluck(...keys), share())
