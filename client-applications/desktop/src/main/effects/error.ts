/**
 * Copyright Fractal Computers, Inc. 2021
 * @file app.ts
 * @brief This file contains subscriptions to error Observables.
 */

import {
  mandelboxCreateErrorNoAccess,
  mandelboxCreateErrorUnauthorized,
  mandelboxCreateErrorMaintenance,
} from "@app/utils/mandelbox"
import { createErrorWindow } from "@app/utils/windows"
import {
  NO_PAYMENT_ERROR,
  UNAUTHORIZED_ERROR,
  MANDELBOX_INTERNAL_ERROR,
  AUTH_ERROR,
  MAINTENANCE_ERROR,
  PROTOCOL_ERROR,
} from "@app/utils/error"
import { fromTrigger } from "@app/utils/flows"

// For any failure, close all windows and display error window
fromTrigger("mandelboxFlowFailure").subscribe((x: any) => {
  if (mandelboxCreateErrorNoAccess(x)) {
    createErrorWindow(NO_PAYMENT_ERROR)
  } else if (mandelboxCreateErrorUnauthorized(x)) {
    createErrorWindow(UNAUTHORIZED_ERROR)
  } else if (mandelboxCreateErrorMaintenance(x)) {
    createErrorWindow(MAINTENANCE_ERROR)
  } else {
    createErrorWindow(MANDELBOX_INTERNAL_ERROR)
  }
})

fromTrigger("authFlowFailure").subscribe(() => {
  createErrorWindow(AUTH_ERROR)
})

fromTrigger("windowInfo").subscribe((args: { crashed: boolean }) => {
  if (args.crashed) createErrorWindow(PROTOCOL_ERROR)
})

fromTrigger("stripePaymentError").subscribe(() => {
  createErrorWindow(NO_PAYMENT_ERROR)
})
