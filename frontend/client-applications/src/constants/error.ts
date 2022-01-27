/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file error.ts
 * @brief This file contains various error messages.
 */

export const NO_PAYMENT_ERROR = "error&type=NO_PAYMENT_ERROR"
export const UNAUTHORIZED_ERROR = "error&type=UNAUTHORIZED_ERROR"
export const PROTOCOL_ERROR = "error&type=PROTOCOL_ERROR"
export const MANDELBOX_INTERNAL_ERROR = "error&type=MANDELBOX_INTERNAL_ERROR"
export const COMMIT_HASH_MISMATCH = "error&type=COMMIT_HASH_MISMATCH"
export const COULD_NOT_LOCK_INSTANCE = "error&type=COULD_NOT_LOCK_INSTANCE"
export const NO_INSTANCE_AVAILABLE = "error&type=NO_INSTANCE_AVAILABLE"
export const REGION_NOT_ENABLED = "error&type=REGION_NOT_ENABLED"
export const USER_ALREADY_ACTIVE = "error&type=USER_ALREADY_ACTIVE"
export const AUTH_ERROR = "error&type=AUTH_ERROR"
export const NAVIGATION_ERROR = "error&type=NAVIGATION_ERROR"
export const MAINTENANCE_ERROR = "error&type=MAINTENANCE_ERROR"
export const LOCATION_CHANGED_ERROR = "error&type=LOCATION_CHANGED_ERROR"
export const SERVER_TIMEOUT_ERROR = "error&type=SERVER_TIMEOUT_ERROR"

export const whistError = {
  [NO_PAYMENT_ERROR]: {
    title: "Your free trial has expired!",
    text: "To continue receiving access to Whist, please sign up for a paid plan.",
  },
  [UNAUTHORIZED_ERROR]: {
    title: "There was an error authenticating you with Whist.",
    text: "Please try logging in again.",
  },
  [PROTOCOL_ERROR]: {
    title: "The Whist browser lost connection.",
    text: "This could be due to inactivity or weak Internet.",
  },
  [MANDELBOX_INTERNAL_ERROR]: {
    title: "There was an error connecting to the Whist servers.",
    text: "Please check your Internet connection or try again in a few minutes.",
  },
  [AUTH_ERROR]: {
    title: "We've added extra security measures to our login system.",
    text: "Please sign out and sign back in.",
  },
  [NAVIGATION_ERROR]: {
    title: "There was an error loading the Whist window.",
    text: "Please try logging in.",
  },
  [MAINTENANCE_ERROR]: {
    title: "Whist is currently pushing out an update.",
    text: "We apologize for the inconvenience. Please check back in a few minutes!",
  },
  [COULD_NOT_LOCK_INSTANCE]: {
    title: "Whist encountered an unexpected database error :(",
    text: "We deeply apologize for the inconvenience. Retrying could cause this error to go away.",
  },
  [NO_INSTANCE_AVAILABLE]: {
    title: "Our servers are at capacity :(",
    text: "We deeply apologize for the inconvenience. We may have more capacity if you try again in a minute.",
  },
  [REGION_NOT_ENABLED]: {
    title: "You are in a region that is not currently supported by Whist :(",
    text: "If you are located in North America, this may be our mistake.",
  },
  [USER_ALREADY_ACTIVE]: {
    title: "You are connected to Whist on another device :(",
    text: "If you believe this is a mistake, we deeply apologize.",
  },
  [COMMIT_HASH_MISMATCH]: {
    title: "Whist version is out of date :(",
    text: "Please allow a few seconds for an update to download.",
  },
  [MANDELBOX_INTERNAL_ERROR]: {
    title: "Unexpected server error :(",
    text: "This is likely a temporary bug and we deeply apologize for the inconvenience.",
  },
  [LOCATION_CHANGED_ERROR]: {
    title: "Your location seems to have changed",
    text: "Since the last time you opened Whist. We recommend restarting Whist to avoid higher network latency.",
  },
  [SERVER_TIMEOUT_ERROR]: {
    title: "Our servers are unresponsive :(",
    text: "You have encountered a rare bug and we deeply apologize. Sometimes this bug will disappear if you try again.",
  },
} as {
  [key: string]: {
    title: string
    text: string
  }
}
