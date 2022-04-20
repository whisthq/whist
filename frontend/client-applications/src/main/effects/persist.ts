/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file app.ts
 * @brief This file contains effects that deal with electron-store
 */

import { merge, filter, of } from "rxjs"
import toPairs from "lodash.topairs"
import find from "lodash.find"

import { fromTrigger } from "@app/main/utils/flows"
import { persistGet, persistSet } from "@app/main/utils/persist"
import { WhistTrigger } from "@app/constants/triggers"
import {
  AWS_REGIONS_SORTED_BY_PROXIMITY,
  RESTORE_LAST_SESSION,
  WHIST_IS_DEFAULT_BROWSER,
  ONBOARDED,
  ALLOW_NON_US_SERVERS,
  GEOLOCATION,
} from "@app/constants/store"
import { withAppActivated } from "@app/main/utils/observables"
import {
  getGeolocation,
  closestRegionHasChanged,
} from "@app/main/utils/location"

// On a successful auth, store the auth credentials in Electron store
// so the user is remembered
merge(
  fromTrigger(WhistTrigger.authFlowSuccess),
  fromTrigger(WhistTrigger.authRefreshSuccess),
  fromTrigger(WhistTrigger.checkPaymentFlowSuccess)
).subscribe(
  (args: {
    userEmail?: string
    accessToken?: string
    refreshToken?: string
    configToken?: string
  }) => {
    toPairs(args).forEach(([key, value]) => {
      if (value !== undefined && value !== "") persistSet(`auth.${key}`, value)
    })
  }
)

// Cache the closest AWS regions so we don't have to wait on AWS ping on subsequent launches
fromTrigger(WhistTrigger.awsPingRefresh).subscribe((regions) => {
  const validPingTimes =
    find(regions, (r) => r.pingTime === undefined) === undefined

  if (regions?.length > 0 && validPingTimes)
    persistSet(AWS_REGIONS_SORTED_BY_PROXIMITY, regions)
})

// Cache whether the user wants their tabs to be restored on launch
fromTrigger(WhistTrigger.restoreLastSession).subscribe(
  (body: { restore: boolean }) => {
    persistSet(RESTORE_LAST_SESSION, body.restore)
  }
)

// Cache whether the user wants Whist as their default browser
fromTrigger(WhistTrigger.setDefaultBrowser).subscribe(
  (body: { default: boolean }) => {
    persistSet(WHIST_IS_DEFAULT_BROWSER, body.default)
  }
)

// If the user is not onboarded but is requesting a mandelbox, this means
// they completed onboarding
fromTrigger(WhistTrigger.protocolConnection)
  .pipe(filter((connected: boolean) => connected))
  .subscribe(() => {
    if (!((persistGet(ONBOARDED) as boolean) ?? false)) {
      persistSet(ONBOARDED, true)
      persistSet(RESTORE_LAST_SESSION, true)
      persistSet(ALLOW_NON_US_SERVERS, false)
    }
  })

fromTrigger(WhistTrigger.allowNonUSServers).subscribe(
  (body: { allow: boolean }) => {
    persistSet(ALLOW_NON_US_SERVERS, body.allow)
  }
)

/* eslint-disable @typescript-eslint/no-misused-promises */
withAppActivated(of(persistGet(GEOLOCATION))).subscribe(async (location) => {
  if (location === undefined) {
    getGeolocation()
      .then((l) => {
        persistSet(GEOLOCATION, JSON.stringify(l))
      })
      .catch((err) => console.error(err))
  }
})

fromTrigger(WhistTrigger.awsPingRefresh).subscribe((regions) => {
  if (closestRegionHasChanged(regions)) {
    getGeolocation()
      .then((l) => {
        persistSet(GEOLOCATION, JSON.stringify(l))
      })
      .catch((err) => console.error(err))
  }
})
