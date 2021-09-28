/**
 * Copyright Fractal Computers, Inc. 2021
 * @file constants.ts
 * @brief This file contains constant values that are passed around to other parts of the application.
 */
import { AWSRegion } from "@app/@types/aws"

export const allowPayments = true

export const HostServicePort = 4678

// The Electron BrowserWindow API can be passed a hash parameter as data.
// We use this so that renderer threads can decide which view component to
// render as soon as a window appears.
export const WindowHashAuth = "AUTH"
export const WindowHashSignout = "SIGNOUT"
export const WindowHashPayment = "PAYMENT"
export const WindowHashProtocol = "PROTOCOL"
export const WindowHashExitTypeform = "EXIT_TYPEFORM"
export const WindowHashBugTypeform = "BUG_TYPEFORM"
export const WindowHashOnboardingTypeform = "ONBOARDING_TYPEFORM"
export const WindowHashSpeedtest = "SPEEDTEST"
export const WindowHashSleep = "SLEEP"
export const WindowHashLoading = "LOADING"
export const WindowHashUpdate = "UPDATE"

export const StateChannel = "MAIN_STATE_CHANNEL"

export const ErrorIPC = [
  "Before you call useMainState(),",
  "an ipcRenderer must be attached to the renderer window object to",
  "communicate with the main process.",
  "\n\nYou need to attach it in an Electron preload.js file using",
  "contextBridge.exposeInMainWorld. You must explicity attach the 'on' and",
  "'send' methods for them to be exposed.",
].join(" ")

export const sessionID = Date.now()

export const defaultAllowedRegions = [
  AWSRegion.US_EAST_1,
  AWSRegion.US_EAST_2,
  AWSRegion.US_WEST_1,
  AWSRegion.US_WEST_2,
  AWSRegion.CA_CENTRAL_1,
]
