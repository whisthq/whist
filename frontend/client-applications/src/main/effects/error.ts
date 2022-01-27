/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file app.ts
 * @brief This file contains effects that create error windows
 */

import {
  NO_PAYMENT_ERROR,
  UNAUTHORIZED_ERROR,
  MANDELBOX_INTERNAL_ERROR,
  AUTH_ERROR,
  MAINTENANCE_ERROR,
  PROTOCOL_ERROR,
  LOCATION_CHANGED_ERROR,
  SERVER_TIMEOUT_ERROR,
  NO_INSTANCE_AVAILABLE,
} from "@app/constants/error"
import {
  WindowHashPayment,
  WindowHashLaunchLoading,
  WindowHashImport,
  WindowHashAuth,
  WindowHashOnboarding,
  WindowHashImportLoading,
} from "@app/constants/windows"

import {
  mandelboxCreateErrorNoAccess,
  mandelboxCreateErrorUnauthorized,
  mandelboxCreateErrorMaintenance,
  mandelboxCreateErrorUnavailable,
} from "@app/main/utils/mandelbox"
import { createErrorWindow } from "@app/main/utils/renderer"
import { destroyElectronWindow } from "@app/main/utils/windows"
import { fromTrigger } from "@app/main/utils/flows"
import {
  withAppActivated,
  untilUpdateAvailable,
  waitForSignal,
} from "@app/main/utils/observables"
import { WhistTrigger } from "@app/constants/triggers"
import { closestRegionHasChanged } from "@app/main/utils/region"

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
  } else if (mandelboxCreateErrorUnavailable(x)) {
    createErrorWindow(NO_INSTANCE_AVAILABLE)
  } else {
    createErrorWindow(MANDELBOX_INTERNAL_ERROR)
  }

  destroyElectronWindow(WindowHashImportLoading)
  destroyElectronWindow(WindowHashLaunchLoading)
  destroyElectronWindow(WindowHashImport)
  destroyElectronWindow(WindowHashOnboarding)
})

untilUpdateAvailable(
  withAppActivated(fromTrigger(WhistTrigger.authFlowFailure))
).subscribe(() => {
  createErrorWindow(AUTH_ERROR)

  destroyElectronWindow(WindowHashAuth)
})

untilUpdateAvailable(
  withAppActivated(fromTrigger(WhistTrigger.stripePaymentError))
).subscribe(() => {
  createErrorWindow(NO_PAYMENT_ERROR)

  destroyElectronWindow(WindowHashPayment)
})

untilUpdateAvailable(
  withAppActivated(fromTrigger(WhistTrigger.protocolError))
).subscribe(() => {
  createErrorWindow(PROTOCOL_ERROR)

  destroyElectronWindow(WindowHashImportLoading)
  destroyElectronWindow(WindowHashLaunchLoading)
  destroyElectronWindow(WindowHashOnboarding)
  destroyElectronWindow(WindowHashImport)
})

// If we detect that the user to a location where another datacenter is closer
// than the one we cached, we show them a warning to encourage them to relaunch Whist
waitForSignal(
  fromTrigger(WhistTrigger.awsPingRefresh),
  fromTrigger(WhistTrigger.authRefreshSuccess)
).subscribe((regions) => {
  if (closestRegionHasChanged(regions)) {
    // The closest AWS regions are different and ping times are more than 25ms apart,
    // show the user a warning window to relaunch Whist in the new AWS region
    setTimeout(() => {
      createErrorWindow(LOCATION_CHANGED_ERROR)
    }, 5000)
  }
})

withAppActivated(fromTrigger(WhistTrigger.checkPaymentFlowFailure)).subscribe(
  () => {
    createErrorWindow(NO_PAYMENT_ERROR)
    destroyElectronWindow(WindowHashAuth)
  }
)

untilUpdateAvailable(
  withAppActivated(fromTrigger(WhistTrigger.mandelboxFlowTimeout))
).subscribe(() => {
  createErrorWindow(SERVER_TIMEOUT_ERROR)

  destroyElectronWindow(WindowHashImportLoading)
  destroyElectronWindow(WindowHashLaunchLoading)
  destroyElectronWindow(WindowHashImport)
  destroyElectronWindow(WindowHashOnboarding)
})
