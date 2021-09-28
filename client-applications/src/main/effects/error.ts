/**
 * Copyright Fractal Computers, Inc. 2021
 * @file app.ts
 * @brief This file contains subscriptions to error Observables.
 */
import { Observable } from "rxjs"
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
import { fromSignal } from "@app/utils/observables"

const withAppReady = (obs: Observable<any>) =>
  fromSignal(obs, fromTrigger("appReady"))

// For any failure, close all windows and display error window
withAppReady(
  fromTrigger("mandelboxFlowFailure").pipe(
    withLatestFrom(
      fromTrigger("updateAvailable").pipe(mapTo(true), startWith(false))
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

withAppReady(fromTrigger("authFlowFailure")).subscribe(() => {
  createErrorWindow(AUTH_ERROR)
})

withAppReady(fromTrigger("stripePaymentError")).subscribe(() => {
  createErrorWindow(NO_PAYMENT_ERROR)
})

withAppReady(fromTrigger("protocolError"))
  .pipe(
    withLatestFrom(
      fromTrigger("updateAvailable").pipe(mapTo(true), startWith(false))
    )
  )
  .subscribe(([, updateAvailable]: [any, boolean]) => {
    if (updateAvailable) {
      createUpdateWindow()
    } else {
      createErrorWindow(PROTOCOL_ERROR)
    }
  })
