/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file app.ts
 * @brief This file contains subscriptions to error Observables.
 */
import {
  mandelboxCreateErrorNoAccess,
  mandelboxCreateErrorUnauthorized,
  mandelboxCreateErrorMaintenance,
} from "@app/main/utils/mandelbox"
import { createErrorWindow } from "@app/main/utils/renderer"
import {
  NO_PAYMENT_ERROR,
  UNAUTHORIZED_ERROR,
  MANDELBOX_INTERNAL_ERROR,
  AUTH_ERROR,
  MAINTENANCE_ERROR,
  PROTOCOL_ERROR,
} from "@app/constants/error"
import { fromTrigger } from "@app/main/utils/flows"
import {
  withAppActivated,
  untilUpdateAvailable,
} from "@app/main/utils/observables"
import { WhistTrigger } from "@app/constants/triggers"

// For any failure, close all windows and display error window
untilUpdateAvailable(
  withAppActivated(fromTrigger(WhistTrigger.mandelboxFlowFailure))
).subscribe((x) => {
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

untilUpdateAvailable(
  withAppActivated(fromTrigger(WhistTrigger.authFlowFailure))
).subscribe(() => {
  createErrorWindow(AUTH_ERROR)
})

untilUpdateAvailable(
  withAppActivated(fromTrigger(WhistTrigger.stripePaymentError))
).subscribe(() => {
  createErrorWindow(NO_PAYMENT_ERROR)
})

untilUpdateAvailable(
  withAppActivated(fromTrigger(WhistTrigger.protocolError))
).subscribe(() => {
  createErrorWindow(PROTOCOL_ERROR)
})
