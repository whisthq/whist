/**
 * Copyright Fractal Computers, Inc. 2021
 * @file error.ts
 * @brief This file contains various error messages.
 */

import { allowPayments } from "@app/utils/constants"

export const NO_PAYMENT_ERROR = "NO_PAYMENT_ERROR"
export const UNAUTHORIZED_ERROR = "UNAUTHORIZED_ERROR"
export const PROTOCOL_ERROR = "PROTOCOL_ERROR"
export const MANDELBOX_INTERNAL_ERROR = "MANDELBOX_INTERNAL_ERROR"
export const AUTH_ERROR = "AUTH_ERROR"
export const NAVIGATION_ERROR = "NAVIGATION_ERROR"
export const MAINTENANCE_ERROR = "MAINTENANCE_ERROR"
export const INTERNET_ERROR = "INTERNET_ERROR"

export const fractalError = {
  [NO_PAYMENT_ERROR]: {
    title: "Your free trial has expired!",
    text: allowPayments
      ? "To continue receiving access to Fractal, please sign up for a paid plan."
      : "To continue receiving access to Fractal, please contact support@fractal.co",
  },
  [UNAUTHORIZED_ERROR]: {
    title: "There was an error authenticating you with Fractal.",
    text: "Please try logging in again or contact support@fractal.co for help.",
  },
  [PROTOCOL_ERROR]: {
    title: "The Fractal browser lost connection.",
    text: "This could be due to inactivity or weak Internet. Please try again or contact support@fractal.co for help.",
  },
  [MANDELBOX_INTERNAL_ERROR]: {
    title: "There was an error connecting to the Fractal servers.",
    text: "Please try again in a few minutes or contact support@fractal.co for help.",
  },
  [AUTH_ERROR]: {
    title: "We've added extra security measures to our login system.",
    text: "Please sign out and sign back in. If this doesn't work, please contact support@fractal.co to report a bug.",
  },
  [NAVIGATION_ERROR]: {
    title: "There was an error loading the Fractal window.",
    text: "Please try logging in again or contact support@fractal.co for help.",
  },
  [MAINTENANCE_ERROR]: {
    title: "Fractal is currently pushing out an update.",
    text: "We apologize for the inconvenience. Please check back in a few minutes!",
  },
  [INTERNET_ERROR]: {
    title: "Please check your Internet connection.",
    text: "We were unable to ping our servers, which is likely a result of weak Internet.",
  },
} as {
  [key: string]: { title: string; text: string }
}
