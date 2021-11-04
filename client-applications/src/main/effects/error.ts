/**
 * Copyright 2021 Fractal Computers, Inc., dba Whist
 * @file app.ts
 * @brief This file contains subscriptions to error Observables.
 */
import { withLatestFrom, startWith, mapTo } from "rxjs/operators"

import {
  mandelboxCreateErrorNoAccess,
  mandelboxCreateErrorUnauthorized,
  mandelboxCreateErrorMaintenance,
} from "@app/utils/mandelbox"
import { createErrorWindow, createUpdateWindow } from "@app/utils/windows"
import {
  NO_PAYMENT_ERROR,
  UNAUTHORIZED_ERROR,
  MANDELBOX_INTERNAL_ERROR,
  AUTH_ERROR,
  MAINTENANCE_ERROR,
  PROTOCOL_ERROR,
} from "@app/utils/error"
import { fromTrigger } from "@app/utils/flows"
import { withAppReady } from "@app/utils/observables"
import { WhistTrigger } from "@app/constants/triggers"

// For any failure, close all windows and display error window
withAppReady(
  fromTrigger(WhistTrigger.mandelboxFlowFailure).pipe(
    withLatestFrom(
      fromTrigger(WhistTrigger.updateAvailable).pipe(
        mapTo(true),
        startWith(false)
      )
    )
  )
).subscribe(([x, updateAvailable]: [any, boolean]) => {
  if (updateAvailable) {
    createUpdateWindow()
  } else {
    if (mandelboxCreateErrorNoAccess(x)) {
      createErrorWindow(NO_PAYMENT_ERROR)
    } else if (mandelboxCreateErrorUnauthorized(x)) {
      createErrorWindow(UNAUTHORIZED_ERROR)
    } else if (mandelboxCreateErrorMaintenance(x)) {
      createErrorWindow(MAINTENANCE_ERROR)
    } else {
      createErrorWindow(MANDELBOX_INTERNAL_ERROR)
    }
  }
})

withAppReady(fromTrigger(WhistTrigger.authFlowFailure)).subscribe(() => {
  createErrorWindow(AUTH_ERROR)
})

withAppReady(fromTrigger(WhistTrigger.stripePaymentError)).subscribe(() => {
  createErrorWindow(NO_PAYMENT_ERROR)
})

withAppReady(fromTrigger(WhistTrigger.protocolError))
  .pipe(
    withLatestFrom(
      fromTrigger(WhistTrigger.updateAvailable).pipe(
        mapTo(true),
        startWith(false)
      )
    )
  )
  .subscribe(([, updateAvailable]: [any, boolean]) => {
    if (updateAvailable) {
      createUpdateWindow()
    } else {
      createErrorWindow(PROTOCOL_ERROR)
    }
  })
