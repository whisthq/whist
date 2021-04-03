/**
 * Copyright Fractal Computers, Inc. 2021
 * @file ipc.ts
 * @brief This file contains subscriptions to Observables related to state persistence.
 */
import { eventIPC } from "@app/main/events/ipc"
import { mapValues } from "lodash"
import { ipcBroadcast } from "@app/utils/state"
import { combineLatest, Observable } from "rxjs"
import { startWith, mapTo, withLatestFrom } from "rxjs/operators"
import { WarningLoginInvalid } from "@app/utils/constants"
import { getWindows } from "@app/utils/windows"
import { loginLoading, loginWarning } from "@app/main/observables/login"

// This file is responsible for broadcasting state to all renderer windows.
// We use a single object and IPC for all windows, so here we set up a single
// observable subscription that calls ipcBroadcast whenever it emits.
//
// ipcBroadcast takes the latest object sent back from the renderer thread,
// and merges it with the latest data from the observables in the "subscribed"
// map below.
//
// Note that combineLatest doesn't emit until each of its observable arguments
// emits an initial value. To get the state broacasting right away, we pipe
// all the subscribed observables through startWith(undefined).
//
// We can only send serializable values over IPC, so the subscribed map is
// constrained to observables that emit serializable values.

type SubscriptionMap = {
    [key: string]: Observable<string | number | boolean | SubscriptionMap>
}

const subscribed: SubscriptionMap = {
    loginLoading: loginLoading,
    loginWarning: loginWarning.pipe(mapTo(WarningLoginInvalid)),
}

combineLatest(
    mapValues(subscribed, (o: any): any => o.pipe(startWith(undefined)))
)
    .pipe(withLatestFrom(eventIPC))
    .subscribe(([subs, state]) =>
        ipcBroadcast({ ...state, ...subs }, getWindows())
    )
