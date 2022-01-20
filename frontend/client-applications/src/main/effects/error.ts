/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file app.ts
 * @brief This file contains effects that create error windows
 */

import find from "lodash.find"

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
  LOCATION_CHANGED_ERROR,
} from "@app/constants/error"
import { fromTrigger } from "@app/main/utils/flows"
import {
  withAppActivated,
  untilUpdateAvailable,
  waitForSignal,
} from "@app/main/utils/observables"
import { WhistTrigger } from "@app/constants/triggers"
import { AWS_REGIONS_SORTED_BY_PROXIMITY } from "@app/constants/store"
import { AWSRegion } from "@app/@types/aws"
import { persistGet } from "@app/main/utils/persist"

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

// If we detect that the user to a location where another datacenter is closer
// than the one we cached, we show them a warning to encourage them to relaunch Whist
waitForSignal(
  fromTrigger(WhistTrigger.awsPingRefresh),
  fromTrigger(WhistTrigger.authRefreshSuccess)
).subscribe((regions) => {
  const previousCachedRegions = persistGet(
    AWS_REGIONS_SORTED_BY_PROXIMITY
  ) as Array<{ region: AWSRegion }>

  const previousClosestRegion = previousCachedRegions?.[0]?.region
  const currentClosestRegion = regions?.[0]?.region

  if (previousClosestRegion === undefined || currentClosestRegion === undefined)
    return

  // If the cached closest AWS region and new closest AWS region are the same, don't do anything
  if (previousClosestRegion === currentClosestRegion) return

  // If the difference in ping time to the cached closest AWS region vs. ping time
  // to the new closest AWS region is less than 25ms, don't do anything
  const previousClosestRegionPingTime = find(
    regions,
    (r) => r.region === previousClosestRegion
  )?.pingTime

  const currentClosestRegionPingTime = regions?.[0]?.pingTime

  if (previousClosestRegionPingTime - currentClosestRegionPingTime < 25) return

  // The closest AWS regions are different and ping times are more than 25ms apart,
  // show the user a warning window to relaunch Whist in the new AWS region
  setTimeout(() => {
    createErrorWindow(LOCATION_CHANGED_ERROR)
  }, 5000)
})
