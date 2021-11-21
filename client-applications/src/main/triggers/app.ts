/**
 * Copyright 2021 Whist Technologies, Inc.
 * @file app.ts
 * @brief This file contains all RXJS observables created from Electron app event emitters.
 */

import { app } from "electron"
import EventEmitter from "events"
import { fromEvent } from "rxjs"

import { createTrigger } from "@app/utils/flows"
import { windowMonitor } from "@app/utils/windows"
import { WhistTrigger } from "@app/constants/triggers"

// Fires when Electron starts; this is the first event to fire
createTrigger(WhistTrigger.appReady, fromEvent(app as EventEmitter, "ready"))
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
  fromEvent(app as EventEmitter, "window-all-closed")
)
