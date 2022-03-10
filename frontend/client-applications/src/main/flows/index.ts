import { merge, of, combineLatest, zip, from } from "rxjs"
import {
  map,
  take,
  filter,
  switchMap,
  share,
  mapTo,
  pluck,
  withLatestFrom,
} from "rxjs/operators"
import isEmpty from "lodash.isempty"
import pickBy from "lodash.pickby"

import authFlow, { authRefreshFlow } from "@app/main/flows/auth"
import checkPaymentFlow from "@app/main/flows/payment"
import mandelboxFlow from "@app/main/flows/mandelbox"
import autoUpdateFlow from "@app/main/flows/autoupdate"
import awsPingFlow from "@app/main/flows/ping"
import { fromTrigger, createTrigger } from "@app/main/utils/flows"
import {
  waitForSignal,
  emitOnSignal,
  withAppActivated,
} from "@app/main/utils/observables"
import { persistGet } from "@app/main/utils/persist"
import { WhistTrigger } from "@app/constants/triggers"
import {
  sleep,
  refreshToken,
  isNewConfigToken,
  darkMode,
  timezone,
  keyRepeat,
  initialKeyRepeat,
} from "@app/main/utils/state"
import {
  CACHED_CONFIG_TOKEN,
  CACHED_REFRESH_TOKEN,
  CACHED_USER_EMAIL,
  ONBOARDED,
  CACHED_ACCESS_TOKEN,
} from "@app/constants/store"
import {
  getDecryptedCookies,
  getBookmarks,
  getExtensions,
  InstalledBrowser,
} from "@app/main/utils/importer"

// Autoupdate flow
const update = autoUpdateFlow(fromTrigger(WhistTrigger.updateAvailable))

// AWS ping flow
const awsPing = awsPingFlow(
  merge(of(null), fromTrigger(WhistTrigger.startNetworkAnalysis))
)

const loggedInAuth = authFlow(
  emitOnSignal(
    of({
      accessToken: persistGet(CACHED_ACCESS_TOKEN) ?? "",
      refreshToken: persistGet(CACHED_REFRESH_TOKEN) ?? "",
      userEmail: persistGet(CACHED_USER_EMAIL) ?? "",
      configToken: persistGet(CACHED_CONFIG_TOKEN) ?? "",
    }).pipe(filter((auth) => isEmpty(pickBy(auth, (x) => x === "")))),
    merge(
      sleep.pipe(filter((sleep) => !sleep)),
      fromTrigger(WhistTrigger.reactivated)
    )
  )
)

const firstAuth = authFlow(fromTrigger(WhistTrigger.authInfo))

// Import all bookmarks and settings
waitForSignal(
  merge(
    fromTrigger(WhistTrigger.beginImport),
    of(persistGet(ONBOARDED)).pipe(filter((onboarded) => onboarded as boolean))
  ),
  fromTrigger(WhistTrigger.authFlowSuccess)
)

// Unpack the access token to see if their payment is valid
const checkPayment = checkPaymentFlow(fromTrigger(WhistTrigger.authFlowSuccess))

// If the payment is invalid, they'll be redirect to the Stripe window. After that they'll
// get new auth credentials
const refreshAfterPaying = authRefreshFlow(
  fromTrigger(WhistTrigger.stripeAuthRefresh).pipe(
    withLatestFrom(
      fromTrigger(WhistTrigger.checkPaymentFlowSuccess).pipe(
        pluck("refreshToken"),
        map((t) => ({ refreshToken: t }))
      )
    ),
    map(([, r]) => r)
  )
)

const dontImportBrowserData = of(persistGet(ONBOARDED) as boolean).pipe(
  take(1),
  filter((onboarded: boolean) => onboarded),
  mapTo(undefined),
  share()
)

const importedData = fromTrigger(WhistTrigger.beginImport).pipe(
  switchMap((t) =>
    zip(
      from(getDecryptedCookies(t?.importBrowserDataFrom as InstalledBrowser)),
      from(getBookmarks(t?.importBrowserDataFrom as InstalledBrowser)),
      from(getExtensions(t?.importBrowserDataFrom as InstalledBrowser))
    )
  ),
  map(([cookies, bookmarks, extensions]) => ({
    cookies,
    bookmarks,
    extensions,
  })),
  share()
)

// Observable that fires when Whist is ready to be launched
const launchTrigger = emitOnSignal(
  combineLatest({
    userEmail: fromTrigger(WhistTrigger.checkPaymentFlowSuccess).pipe(
      pluck("userEmail")
    ),
    accessToken: fromTrigger(WhistTrigger.checkPaymentFlowSuccess).pipe(
      pluck("accessToken")
    ),
    configToken: fromTrigger(WhistTrigger.checkPaymentFlowSuccess).pipe(
      pluck("configToken")
    ),
    isNewConfigToken,
    importedData: merge(importedData, dontImportBrowserData),
    regions: merge(awsPing.cached, awsPing.refresh),
    darkMode,
    timezone,
    keyRepeat,
    initialKeyRepeat,
  }),
  merge(
    zip(
      fromTrigger(WhistTrigger.checkPaymentFlowSuccess),
      of(persistGet(ONBOARDED)).pipe(
        filter((onboarded) => onboarded as boolean)
      )
    ), // On a normal launch
    importedData // On onboarding or import
  )
).pipe(share())

// Mandelbox creation flow
const mandelbox = mandelboxFlow(withAppActivated(launchTrigger))

// After the mandelbox flow is done, run the refresh flow so the tokens are being refreshed
// every time but don't impede startup time
const refreshAtEnd = authRefreshFlow(
  emitOnSignal(combineLatest({ refreshToken }), mandelbox.success)
)

createTrigger(WhistTrigger.checkPaymentFlowSuccess, checkPayment.success)
createTrigger(WhistTrigger.checkPaymentFlowFailure, checkPayment.failure)

createTrigger(WhistTrigger.mandelboxFlowStart, launchTrigger)

createTrigger(WhistTrigger.awsPingCached, awsPing.cached)
createTrigger(WhistTrigger.awsPingRefresh, awsPing.refresh)
createTrigger(WhistTrigger.awsPingOffline, awsPing.offline)

createTrigger(
  WhistTrigger.authFlowSuccess,
  merge(loggedInAuth.success, firstAuth.success)
)
createTrigger(
  WhistTrigger.authFlowFailure,
  merge(loggedInAuth.failure, firstAuth.failure)
)

createTrigger(WhistTrigger.updateDownloaded, update.downloaded)
createTrigger(WhistTrigger.downloadProgress, update.progress)

createTrigger(
  WhistTrigger.authRefreshSuccess,
  merge(refreshAtEnd.success, refreshAfterPaying.success)
)

createTrigger(WhistTrigger.mandelboxFlowSuccess, mandelbox.success)
createTrigger(WhistTrigger.mandelboxFlowFailure, mandelbox.failure)
createTrigger(WhistTrigger.mandelboxFlowTimeout, mandelbox.timeout)
