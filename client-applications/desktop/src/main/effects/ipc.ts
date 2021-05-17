/**
 * Copyright Fractal Computers, Inc. 2021
 * @file ipc.ts
 * @brief This file contains subscriptions to Observables related to state persistence.
 */
import { ipcBroadcast } from "@app/utils/ipc"
import { StateIPC } from "@app/@types/state"
import { map, mapTo, withLatestFrom, startWith } from "rxjs/operators"

import { WarningLoginInvalid, WarningSignupInvalid } from "@app/utils/constants"
import { getWindows } from "@app/utils/windows"
import { objectCombine } from "@app/utils/observables"
import { fromTrigger } from "@app/utils/flows"

// This file is responsible for broadcasting state to all renderer windows.
// We use a single object and IPC channel for all windows, so here we set up a
// single observable subscription that calls ipcBroadcast whenever it emits.
//
// This subscription takes the latest object sent back from the renderer thread,
// and merges it with the latest data from the observables in the "subscribed"
// map below. It sends the result to all windows with ipcBroadcast.
//
// Note that combineLatest doesn't emit until each of its observable arguments
// emits an initial value. To get the state broadcasting right away, we pipe
// all the subscribed observables through startWith(undefined).
//
// We can only send serializable values over IPC, so the subscribed map is
// constrained to observables that emit serializable values.

const subscribed = {
  // loginLoading: loginLoading,
  // loginWarning: loginWarning.pipe(mapTo(WarningLoginInvalid)),
  updateInfo: fromTrigger("downloadProgress").pipe(map((obj) => JSON.stringify(obj))),
  // signupLoading: signupLoading,
  // signupWarning: signupWarning.pipe(mapTo(WarningSignupInvalid)),
}

objectCombine(subscribed)
  .pipe(withLatestFrom(fromTrigger("eventIPC").pipe(startWith({}))))
  .subscribe(([subs, state]: [Partial<StateIPC>, Partial<StateIPC>]) => {
    ipcBroadcast({ ...state, ...subs } as Partial<StateIPC>, getWindows())
  })
