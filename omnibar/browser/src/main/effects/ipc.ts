/**
 * Copyright Fractal Computers, Inc. 2021
 * @file ipc.ts
 * @brief This file contains subscriptions to Observables related to state persistence.
 */
import { BrowserWindow, BrowserView } from "electron";
import { combineLatest, merge } from "rxjs";
import { mapTo, withLatestFrom, startWith, map } from "rxjs/operators";
import { ipcBroadcast } from "@app/utils/state/ipc";
import { StateIPC } from "@app/utils/constants/state";

import { fromTrigger } from "@app/utils/rxjs/triggers";
import { FractalTrigger } from "@app/utils/constants/triggers";
import { getBrowserWindow } from "@app/utils/navigation/browser";
import { fromSignal } from "@app/utils/rxjs/observables";

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

// Fires when a view loads a new URL, sent to the renderer to update tab image
const browserViewChanged = merge(
  fromTrigger(FractalTrigger.viewCreated).pipe(
    mapTo(FractalTrigger.viewCreated)
  ),
  fromTrigger(FractalTrigger.viewDisplayed).pipe(
    mapTo(FractalTrigger.viewDisplayed)
  ),
  fromTrigger(FractalTrigger.viewDestroyed).pipe(
    mapTo(FractalTrigger.viewDestroyed)
  ),
  fromTrigger(FractalTrigger.browserWindowClosed).pipe(
    mapTo(FractalTrigger.viewDestroyed)
  ),
  fromTrigger(FractalTrigger.viewTitleChanged).pipe(
    mapTo(FractalTrigger.viewTitleChanged)
  )
).pipe(
  withLatestFrom(
    merge(
      fromTrigger(FractalTrigger.viewCreated),
      fromTrigger(FractalTrigger.viewDisplayed)
    )
  ),
  map(([name, topView]: [FractalTrigger, BrowserView]) => ({
    name: name,
    payload: {
      views:
        getBrowserWindow()
          ?.getBrowserViews()
          .map((view: BrowserView) => ({
            url: view.webContents?.getURL(),
            title: view.webContents?.getTitle(),
            isOnTop: topView.webContents?.id === view.webContents?.id,
            id: view.webContents?.id,
          })) ?? [],
    },
  }))
);

const persistChanged = fromTrigger(FractalTrigger.persistDidChange);

const subscribed = combineLatest({
  trigger: merge(browserViewChanged).pipe(startWith(undefined)),
  persisted: persistChanged.pipe(startWith([])),
});

fromSignal(
  combineLatest([
    subscribed,
    fromTrigger(FractalTrigger.eventIPC).pipe(startWith({})),
  ]),
  fromTrigger(FractalTrigger.browserWindowCreated)
).subscribe(([subs, state]: [Partial<StateIPC>, Partial<StateIPC>]) => {
  const windows = BrowserWindow.getAllWindows();
  ipcBroadcast({ ...state, ...subs } as Partial<StateIPC>, windows);
});
