/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file app.ts
 * @brief This file contains effects that deal with electron-store
 */

import { merge, filter } from "rxjs"
import toPairs from "lodash.topairs"

import { fromTrigger } from "@app/main/utils/flows"
import { persistGet, persistSet } from "@app/main/utils/persist"
import { WhistTrigger } from "@app/constants/triggers"
import {
  AWS_REGIONS_SORTED_BY_PROXIMITY,
  RESTORE_LAST_SESSION,
  WHIST_IS_DEFAULT_BROWSER,
  ONBOARDED,
} from "@app/constants/store"

// On a successful auth, store the auth credentials in Electron store
// so the user is remembered
merge(
  fromTrigger(WhistTrigger.authFlowSuccess),
  fromTrigger(WhistTrigger.authRefreshSuccess)
).subscribe(
  (args: {
    userEmail?: string
    accessToken?: string
    refreshToken?: string
    configToken?: string
  }) => {
    toPairs(args).forEach(([key, value]) => {
      if (value !== undefined) persistSet(`auth.${key}`, value)
    })
  }
)

// Cache the closest AWS regions so we don't have to wait on AWS ping on subsequent launches
fromTrigger(WhistTrigger.awsPingRefresh).subscribe((regions) => {
  if (regions?.length > 0) persistSet(AWS_REGIONS_SORTED_BY_PROXIMITY, regions)
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
  .pipe(filter((connected) => connected))
  .subscribe(() => {
    if (!((persistGet(ONBOARDED) as boolean) ?? false)) {
      persistSet(ONBOARDED, true)
      persistSet(RESTORE_LAST_SESSION, true)
    }
  })
