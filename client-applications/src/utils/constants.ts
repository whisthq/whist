/**
 * Copyright 2021 Fractal Computers, Inc., dba Whist
 * @file constants.ts
 * @brief This file contains constant values that are passed around to other parts of the application.
 */
import { AWSRegion } from "@app/@types/aws"
import { appEnvironment, FractalEnvironments } from "../../config/configs"

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

// Base logz GET request to apply filters and queries
const PROTOCOL_SERVER_LOGZ_BASE = `https://app.logz.io/#/dashboard/kibana/discover?_a=(columns:!(message),filters:!(('$state':(store:appState),meta:(alias:!n,disabled:!f,index:'logzioCustomerIndex*',key:session_id,negate:!f,params:(query:'${sessionID}'),type:phrase),query:(match_phrase:(session_id:'${sessionID}'))),('$state':(store:appState),meta:(alias:!n,disabled:!f,index:'logzioCustomerIndex*',key:component,negate:!f,params:(query:mandelbox),type:phrase),query:(match_phrase:(component:mandelbox)))),index:'logzioCustomerIndex*',interval:auto,query:(language:lucene,query:''),sort:!(!('@timestamp',desc)))&_g=(filters:!(),refreshInterval:(pause:!t,value:0),time:(from:now-15d,to:now))`
const PROTOCOL_CLIENT_LOGZ_BASE = `https://app.logz.io/#/dashboard/kibana/discover?_a=(columns:!(message),filters:!(('$state':(store:appState),meta:(alias:!n,disabled:!f,index:'logzioCustomerIndex*',key:session_id,negate:!f,params:(query:'${sessionID}'),type:phrase),query:(match_phrase:(session_id:'${sessionID}'))),('$state':(store:appState),meta:(alias:!n,disabled:!f,index:'logzioCustomerIndex*',key:component,negate:!f,params:(query:clientapp),type:phrase),query:(match_phrase:(component:clientapp)))),index:'logzioCustomerIndex*',interval:auto,query:(language:lucene,query:''),sort:!(!('@timestamp',desc)))&_g=(filters:!(),refreshInterval:(pause:!t,value:0),time:(from:now-15d,to:now))`

// GET params to switch logz accounts (dev/staging/prod)
const LOGZ_PROD_PARAMS = "&accountIds=162757&switchToAccountId=162757"
const LOGZ_STAGING_PARAMS = "&switchToAccountId=319255&accountIds=319255"
const LOGZ_DEV_PARAMS = "&accountIds=167279&switchToAccountId=167279"

// Final URL
export const logzProtocolServerURL = () => {
  switch (appEnvironment) {
    case FractalEnvironments.PRODUCTION:
      return `${PROTOCOL_SERVER_LOGZ_BASE}${LOGZ_PROD_PARAMS}`
    case FractalEnvironments.STAGING:
      return `${PROTOCOL_SERVER_LOGZ_BASE}${LOGZ_STAGING_PARAMS}`
    default:
      return `${PROTOCOL_SERVER_LOGZ_BASE}${LOGZ_DEV_PARAMS}`
  }
}

export const logzProtocolClientURL = () => {
  switch (appEnvironment) {
    case FractalEnvironments.PRODUCTION:
      return `${PROTOCOL_CLIENT_LOGZ_BASE}${LOGZ_PROD_PARAMS}`
    case FractalEnvironments.STAGING:
      return `${PROTOCOL_CLIENT_LOGZ_BASE}${LOGZ_STAGING_PARAMS}`
    default:
      return `${PROTOCOL_CLIENT_LOGZ_BASE}${LOGZ_DEV_PARAMS}`
  }
}

// How often to send heartbeat logs to Amplitude
export const LOG_INTERVAL_IN_MINUTES = 5

// Sentry DSN
export const SENTRY_DSN =
  "https://5b0accb25f3341d280bb76f08775efe1@o400459.ingest.sentry.io/5412323"
