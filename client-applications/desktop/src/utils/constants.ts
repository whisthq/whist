/**
 * Copyright Fractal Computers, Inc. 2021
 * @file constants.ts
 * @brief This file contains constant values that are passed around to other parts of the application.
 */

export const allowPayments = false

export const HostServicePort = 4678

// poll for a mandelbox for 30 seconds max
export const mandelboxPollingTimeout = 30000

// The Electron BrowserWindow API can be passed a hash parameter as data.
// We use this so that renderer threads can decide which view component to
// render as soon as a window appears.
export const WindowHashAuth = "AUTH"
export const WindowHashUpdate = "UPDATE"
export const WindowHashSignout = "SIGNOUT"
export const WindowHashPayment = "PAYMENT"
export const WindowHashTypeform = "TYPEFORM"

export const StateChannel = "MAIN_STATE_CHANNEL"

export const ErrorIPC = [
  "Before you call useMainState(),",
  "an ipcRenderer must be attached to the renderer window object to",
  "communicate with the main process.",
  "\n\nYou need to attach it in an Electron preload.js file using",
  "contextBridge.exposeInMainWorld. You must explicity attach the 'on' and",
  "'send' methods for them to be exposed.",
].join(" ")
