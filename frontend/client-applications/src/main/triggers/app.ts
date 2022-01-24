/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file app.ts
 * @brief This file contains all RXJS observables created from Electron app event emitters.
 */

import { app } from "electron"
import { fromEvent } from "rxjs"
import { take, switchMap, filter } from "rxjs/operators"

import { createTrigger } from "@app/main/utils/flows"
import { WhistTrigger } from "@app/constants/triggers"
import { storeEmitter } from "@app/main/utils/persist"
import { sleep } from "@app/main/utils/state"

// Fires when Electron starts; this is the first event to fire
createTrigger(WhistTrigger.appReady, fromEvent(app, "ready"))

// Fires when all Electron windows have been closed
createTrigger(
  WhistTrigger.electronWindowsAllClosed,
  fromEvent(app, "window-all-closed")
)
// Fires when all windows are closed and the user clicks the dock icon to re-open Whist
createTrigger(
  WhistTrigger.reactivated,
  sleep.pipe(
    filter((s) => s),
    switchMap(() => fromEvent(app, "activate").pipe(take(1)))
  )
)

createTrigger(
  WhistTrigger.storeDidChange,
  fromEvent(storeEmitter, "store-did-change")
)
