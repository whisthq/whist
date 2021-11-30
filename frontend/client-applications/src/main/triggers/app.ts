/**
 * Copyright 2021 Whist Technologies, Inc.
 * @file app.ts
 * @brief This file contains all RXJS observables created from Electron app event emitters.
 */

import { app } from "electron"
import { fromEvent } from "rxjs"
import { filter, map, withLatestFrom } from "rxjs/operators"

import { createTrigger, fromTrigger } from "@app/utils/flows"
import { windowMonitor } from "@app/utils/windows"
import { WhistTrigger } from "@app/constants/triggers"

// Fires when Electron starts; this is the first event to fire
createTrigger(WhistTrigger.appReady, fromEvent(app, "ready"))
// Fires whenever the number of windows changes, including the protocol window
createTrigger(WhistTrigger.windowInfo, fromEvent(windowMonitor, "window-info"))
// Fires whenever the network is unstable
createTrigger(
  WhistTrigger.networkUnstable,
  fromEvent(windowMonitor, "network-is-unstable")
)
// Fires when all Electron windows have been closed
createTrigger(
  WhistTrigger.windowsAllClosed,
  fromEvent(app, "window-all-closed")
)
// Fires when all windows are closed and the user clicks the dock icon to re-open Whist
createTrigger(
  WhistTrigger.reactivated,
  fromEvent(app, "activate").pipe(
    withLatestFrom(fromTrigger(WhistTrigger.windowInfo)),
    filter(([, args]) => args.numberWindowsRemaining === 0),
    map(([x]) => x)
  )
)
