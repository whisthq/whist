/**
 * Copyright 2021 Whist Technologies, Inc.
 * @file app.ts
 * @brief This file contains subscriptions to error Observables.
 */
import {
  mandelboxCreateErrorNoAccess,
  mandelboxCreateErrorUnauthorized,
  mandelboxCreateErrorMaintenance,
  mandelboxCreateErrorCommitHash,
} from "@app/utils/mandelbox"
import { createErrorWindow } from "@app/utils/windows"
import {
  NO_PAYMENT_ERROR,
  UNAUTHORIZED_ERROR,
  MANDELBOX_INTERNAL_ERROR,
  AUTH_ERROR,
  MAINTENANCE_ERROR,
  PROTOCOL_ERROR,
  COMMIT_HASH_ERROR,
} from "@app/utils/error"
import { fromTrigger } from "@app/utils/flows"
import { withAppReady } from "@app/utils/observables"
import { WhistTrigger } from "@app/constants/triggers"

// For any failure, close all windows and display error window
withAppReady(fromTrigger(WhistTrigger.mandelboxFlowFailure)).subscribe((x) => {
  if (mandelboxCreateErrorCommitHash(x)) {
    createErrorWindow(COMMIT_HASH_ERROR)
  } else if (mandelboxCreateErrorNoAccess(x)) {
    createErrorWindow(NO_PAYMENT_ERROR)
  } else if (mandelboxCreateErrorUnauthorized(x)) {
    createErrorWindow(UNAUTHORIZED_ERROR)
  } else if (mandelboxCreateErrorMaintenance(x)) {
    createErrorWindow(MAINTENANCE_ERROR)
  } else {
    createErrorWindow(MANDELBOX_INTERNAL_ERROR)
  }
})

withAppReady(fromTrigger(WhistTrigger.authFlowFailure)).subscribe(() => {
  createErrorWindow(AUTH_ERROR)
})

withAppReady(fromTrigger(WhistTrigger.stripePaymentError)).subscribe(() => {
  createErrorWindow(NO_PAYMENT_ERROR)
})

withAppReady(fromTrigger(WhistTrigger.protocolError)).subscribe(() => {
  createErrorWindow(PROTOCOL_ERROR)
})
