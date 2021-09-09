/**
 * Copyright Fractal Computers, Inc. 2021
 * @file app.ts
 * @brief This file contains all RXJS observables created from Electron app event emitters.
 */

import { app } from "electron"
import EventEmitter from "events"
import { fromEvent } from "rxjs"

import { createTrigger } from "@app/utils/flows"
import { windowMonitor } from "@app/utils/windows"
import TRIGGER from "@app/utils/triggers"

// Fires when Electron starts; this is the first event to fire
createTrigger(TRIGGER.appReady, fromEvent(app as EventEmitter, "ready"))
// Fires whenever the number of windows changes, including the protocol window
createTrigger(TRIGGER.windowInfo, fromEvent(windowMonitor, "window-info"))
// Fires whenever the network is unstable
createTrigger(
  TRIGGER.networkUnstable,
  fromEvent(windowMonitor, "network-is-unstable")
)
// Fires when all Electron windows have been closed
createTrigger(
  TRIGGER.windowsAllClosed,
  fromEvent(app as EventEmitter, "window-all-closed")
)
