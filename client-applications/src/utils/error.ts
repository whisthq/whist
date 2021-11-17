/**
 * Copyright 2021 Fractal Computers, Inc., dba Whist
 * @file error.ts
 * @brief This file contains various error messages.
 */

export const NO_PAYMENT_ERROR = "NO_PAYMENT_ERROR"
export const UNAUTHORIZED_ERROR = "UNAUTHORIZED_ERROR"
export const PROTOCOL_ERROR = "PROTOCOL_ERROR"
export const MANDELBOX_INTERNAL_ERROR = "MANDELBOX_INTERNAL_ERROR"
export const AUTH_ERROR = "AUTH_ERROR"
export const NAVIGATION_ERROR = "NAVIGATION_ERROR"
export const MAINTENANCE_ERROR = "MAINTENANCE_ERROR"
export const INTERNET_ERROR = "INTERNET_ERROR"
export const SLEEP_ERROR = "SLEEP_ERROR"

export const fractalError = {
  [NO_PAYMENT_ERROR]: {
    title: "Your free trial has expired!",
    text: "To continue receiving access to Whist, please sign up for a paid plan.",
  },
  [UNAUTHORIZED_ERROR]: {
    title: "There was an error authenticating you with Whist.",
    text: "Please try logging in again or contact support@whist.com for help.",
  },
  [PROTOCOL_ERROR]: {
    title: "The Whist browser lost connection.",
    text: "This could be due to inactivity or weak Internet. Please try again or contact support@whist.com for help.",
  },
  [MANDELBOX_INTERNAL_ERROR]: {
    title: "There was an error connecting to the Whist servers.",
    text: "Please check your Internet connection or try again in a few minutes.",
  },
  [AUTH_ERROR]: {
    title: "We've added extra security measures to our login system.",
    text: "Please sign out and sign back in. If this doesn't work, please contact support@whist.com to report a bug.",
  },
  [NAVIGATION_ERROR]: {
    title: "There was an error loading the Whist window.",
    text: "Please try logging in again or contact support@whist.com for help.",
  },
  [MAINTENANCE_ERROR]: {
    title: "Whist is currently pushing out an update.",
    text: "We apologize for the inconvenience. Please check back in a few minutes!",
  },
  [INTERNET_ERROR]: {
    title: "Please check your Internet connection.",
    text: "We were unable to ping our servers, which is likely a result of weak Internet.",
  },
  [SLEEP_ERROR]: {
    title: "Your computer went to sleep.",
    text: "Whist automatically disconnected and should reconnect when your computer wakes up.",
  },
} as {
  [key: string]: {
    title: string
    text: string
  }
}
