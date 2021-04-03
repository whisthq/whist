/**
 * Copyright Fractal Computers, Inc. 2021
 * @file ipc.ts
 * @brief This file contains subscriptions to Observables related to state persistence.
 */
import { eventIPC } from "@app/main/events/ipc"
import { ipcBroadcast } from "@app/utils/state"
import { combineLatest } from "rxjs"
import { startWith, mapTo } from "rxjs/operators"
import { WarningLoginInvalid } from "@app/utils/constants"
import { getWindows } from "@app/utils/windows"
import { loginLoading, loginWarning } from "@app/main/observables/login"

// Broadcast state to all renderer windows.
combineLatest(
    eventIPC,
    loginLoading.pipe(startWith(false)),
    loginWarning.pipe(mapTo(WarningLoginInvalid), startWith(null))
).subscribe(([state, loginLoading, loginWarning]) =>
    ipcBroadcast({ ...state, loginLoading, loginWarning }, getWindows())
)
