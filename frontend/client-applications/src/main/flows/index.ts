import { merge, of, combineLatest, zip, from } from "rxjs"
import { map, take, filter, switchMap, share, mapTo } from "rxjs/operators"
import isEmpty from "lodash.isempty"
import pickBy from "lodash.pickby"

import authFlow, { authRefreshFlow } from "@app/main/flows/auth"
import checkPaymentFlow from "@app/main/flows/payment"
import mandelboxFlow from "@app/main/flows/mandelbox"
import autoUpdateFlow from "@app/main/flows/autoupdate"
import awsPingFlow from "@app/main/flows/ping"
import { fromTrigger, createTrigger } from "@app/utils/flows"
import { fromSignal } from "@app/utils/observables"
import { getRegionFromArgv } from "@app/utils/region"
import { persistGet } from "@app/utils/persist"
import { WhistTrigger } from "@app/constants/triggers"
import {
  accessToken,
  refreshToken,
  userEmail,
  configToken,
  isNewConfigToken,
} from "@app/utils/state"
import { ONBOARDED } from "@app/constants/store"
import {
  getDecryptedCookies,
  getBookmarks,
  getExtensions,
  InstalledBrowser,
} from "@app/utils/importer"

// Autoupdate flow
const update = autoUpdateFlow(fromTrigger(WhistTrigger.updateAvailable))

// AWS ping flow
const awsPing = awsPingFlow(of(null))

// Auth flow
const auth = authFlow(
  merge(
    fromTrigger(WhistTrigger.authInfo),
    combineLatest({
      accessToken,
      refreshToken,
      userEmail,
      configToken,
    }).pipe(
      take(1),
      filter((auth) => isEmpty(pickBy(auth, (x) => x === "")))
    )
  )
)

// Onboarding flow
const onboarded = fromSignal(
  merge(
    fromTrigger(WhistTrigger.onboarded),
    zip(
      of(persistGet(ONBOARDED)).pipe(
        filter((onboarded) => onboarded as boolean)
      )
    )
  ),
  fromTrigger(WhistTrigger.authFlowSuccess)
)

// Unpack the access token to see if their payment is valid
const checkPayment = checkPaymentFlow(
  fromSignal(combineLatest({ accessToken }), onboarded)
)

// If the payment is invalid, they'll be redirect to the Stripe window. After that they'll
// get new auth credentials
const refreshAfterPaying = authRefreshFlow(
  fromSignal(
    combineLatest({ refreshToken }),
    fromTrigger(WhistTrigger.stripeAuthRefresh)
  )
)

// If importBrowserDataFrom is not empty, run the cookie import function
const importCookies = fromTrigger(WhistTrigger.onboarded).pipe(
  switchMap((t) =>
    from(getDecryptedCookies(t?.importBrowserDataFrom as InstalledBrowser))
  ),
  share() // If you don't share, this observable will fire many times (once for each subscriber of the flow)
)

const importBookmarks = fromTrigger(WhistTrigger.onboarded).pipe(
  switchMap((t) =>
    from(getBookmarks(t?.importBrowserDataFrom as InstalledBrowser))
  ),
  share() // If you don't share, this observable will fire many times (once for each subscriber of the flow)
)

const importExtensions = fromTrigger(WhistTrigger.onboarded).pipe(
  switchMap((t) =>
    from(getExtensions(t?.importBrowserDataFrom as InstalledBrowser))
  ),
  share() // If you don't share, this observable will fire many times (once for each subscriber of the flow)
)

const dontImportBrowserData = of(persistGet(ONBOARDED) as boolean).pipe(
  take(1),
  filter((onboarded: boolean) => onboarded),
  mapTo(undefined),
  share()
)

// Observable that fires when Whist is ready to be launched
const launchTrigger = fromSignal(
  combineLatest({
    userEmail,
    accessToken,
    configToken,
    isNewConfigToken,
    cookies: merge(importCookies, dontImportBrowserData),
    bookmarks: merge(importBookmarks, dontImportBrowserData),
    extensions: merge(importExtensions, dontImportBrowserData),
    regions: merge(awsPing.cached, awsPing.refresh),
  }).pipe(
    map((x: object) => ({
      ...x,
      region: getRegionFromArgv(process.argv),
    }))
  ),
  merge(
    fromTrigger(WhistTrigger.checkPaymentFlowSuccess),
    refreshAfterPaying.success
  )
).pipe(take(1), share())

// Mandelbox creation flow
const mandelbox = mandelboxFlow(launchTrigger)

// After the mandelbox flow is done, run the refresh flow so the tokens are being refreshed
// every time but don't impede startup time
const refreshAtEnd = authRefreshFlow(
  fromSignal(combineLatest({ refreshToken }), mandelbox.success)
)

createTrigger(WhistTrigger.awsPingCached, awsPing.cached)
createTrigger(WhistTrigger.awsPingRefresh, awsPing.refresh)

createTrigger(WhistTrigger.authFlowSuccess, auth.success)
createTrigger(WhistTrigger.authFlowFailure, auth.failure)

createTrigger(WhistTrigger.updateDownloaded, update.downloaded)
createTrigger(WhistTrigger.downloadProgress, update.progress)

createTrigger(WhistTrigger.authRefreshSuccess, refreshAtEnd.success)

createTrigger(WhistTrigger.checkPaymentFlowSuccess, checkPayment.success)
createTrigger(WhistTrigger.checkPaymentFlowFailure, checkPayment.failure)

createTrigger(WhistTrigger.mandelboxFlowSuccess, mandelbox.success)
createTrigger(WhistTrigger.mandelboxFlowFailure, mandelbox.failure)
