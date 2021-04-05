/**
 * Copyright Fractal Computers, Inc. 2021
 * @file app.ts
 * @brief This file contains all RXJS observables created from Electron app event emitters.
 */

import { app } from "electron"
import EventEmitter from "events"
import { fromEvent } from "rxjs"

export const eventAppReady = fromEvent(app as EventEmitter, "ready")

export const eventAppActivation = fromEvent(app as EventEmitter, "activate")

export const eventAppQuit = fromEvent(app as EventEmitter, "window-all-closed")

eventAppReady.subscribe(() => console.log("ready"))
