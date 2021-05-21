/**
 * Copyright Fractal Computers, Inc. 2021
 * @file app.ts
 * @brief This file contains all RXJS observables created from Electron app event emitters.
 */
import { app } from "electron"
import EventEmitter from "events"
import { fromEvent } from "rxjs"
import path from "path"
import config from "@app/config/environment"

// rxjs and Typescript are not fully agreeing on the type inference here,
// so we coerce to EventEmitter to keep everyone happy.

export const eventAppReady = fromEvent(app as EventEmitter, "ready")

export const eventAppActivation = fromEvent(app as EventEmitter, "activate")

export const eventWindowCreated = fromEvent(
  app as EventEmitter,
  "browser-window-created"
)

// Set custom app data folder based on environment
// TODO: In this PR, we must resolve the race condition! When we compute config,
// we depend on knowing app.getPath("userData"), and yet we only properly set that
// path here, in code that depends on config! This causes our log files to be written
// into the default userData path, but everything else to be written into the modified
// userData path.
const { deployEnv } = config
const appPath = app.getPath("userData")
const newPath = path.join(appPath, deployEnv)
app.setPath("userData", newPath)
