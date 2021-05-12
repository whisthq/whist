/**
 * Copyright Fractal Computers, Inc. 2021
 * @file app.ts
 * @brief This file contains all RXJS observables created from Electron app event emitters.
 */
import { app } from "electron"
import EventEmitter from "events"
import { fromEvent } from "rxjs"

import { trigger } from "@app/main/utils/flows"

// rxjs and Typescript are not fully agreeing on the type inference here,
// so we coerce to EventEmitter to keep everyone happy.
export const appReady = trigger("appReady", fromEvent(app as EventEmitter, "ready"))

export const windowCreated = trigger("windowCreated", fromEvent(
  app as EventEmitter,
  "browser-window-created"
))
