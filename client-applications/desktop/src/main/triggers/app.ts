/**
 * Copyright Fractal Computers, Inc. 2021
 * @file app.ts
 * @brief This file contains all RXJS observables created from Electron app event emitters.
 */

import { app } from "electron"
import EventEmitter from "events"
import { fromEvent } from "rxjs"

import { createTrigger } from "@app/utils/flows"
import { windowEmitter } from "@app/utils/windows"
import TRIGGER from "@app/utils/triggers"

// Fires when Electron starts; this is the first event to fire
createTrigger(TRIGGER.appReady, fromEvent(app as EventEmitter, "ready"))
// Fires whenever a BrowserWindow is created
createTrigger(
  TRIGGER.windowCreated,
  fromEvent(app as EventEmitter, "browser-window-created")
)
// Fires whenever a BrowserWindow is closed
createTrigger(
  TRIGGER.windowClosed,
  fromEvent(windowEmitter, "close")
)
// Fires when there are no windows left and Electron wants to quit the app
createTrigger(TRIGGER.willQuit, fromEvent(app as EventEmitter, "will-quit"))
