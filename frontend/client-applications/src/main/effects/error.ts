/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file app.ts
 * @brief This file contains subscriptions to error Observables.
 */
import { merge } from "rxjs"
import { mapTo, withLatestFrom } from "rxjs/operators"

import {
  mandelboxCreateErrorNoAccess,
  mandelboxCreateErrorUnauthorized,
  mandelboxCreateErrorMaintenance,
  mandelboxCreateErrorUnavailable,
} from "@app/utils/mandelbox"
import { createErrorWindow } from "@app/utils/windows"
import {
  NO_PAYMENT_ERROR,
  UNAUTHORIZED_ERROR,
  MANDELBOX_INTERNAL_ERROR,
  AUTH_ERROR,
  MAINTENANCE_ERROR,
  PROTOCOL_ERROR,
} from "@app/constants/error"
import { fromTrigger } from "@app/utils/flows"
import { withAppReady } from "@app/utils/observables"
import { WhistTrigger } from "@app/constants/triggers"

// For any failure, close all windows and display error window
withAppReady(fromTrigger(WhistTrigger.mandelboxFlowFailure))
  .pipe(
    withLatestFrom(
      merge(
        fromTrigger(WhistTrigger.updateAvailable).pipe(mapTo(true)),
        fromTrigger(WhistTrigger.updateNotAvailable).pipe(mapTo(false))
      )
    )
  )
  .subscribe(([x, updateAvailable]) => {
    if (mandelboxCreateErrorNoAccess(x)) {
      createErrorWindow(NO_PAYMENT_ERROR)
    } else if (mandelboxCreateErrorUnauthorized(x)) {
      createErrorWindow(UNAUTHORIZED_ERROR)
    } else if (mandelboxCreateErrorMaintenance(x)) {
      createErrorWindow(MAINTENANCE_ERROR)
    } else if (mandelboxCreateErrorUnavailable(x)) {
      if (x?.json?.error === "COMMIT_HASH_MISMATCH" && updateAvailable) return
      createErrorWindow(x?.json?.error ?? MANDELBOX_INTERNAL_ERROR)
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
