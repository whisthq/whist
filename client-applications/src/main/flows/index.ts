import { merge, of, combineLatest, zip } from "rxjs"
import { map, take, filter, startWith, share } from "rxjs/operators"
import isEmpty from "lodash.isempty"
import pickBy from "lodash.pickby"

import authFlow, { authRefreshFlow } from "@app/main/flows/auth"
import checkPaymentFlow from "@app/main/flows/payment"
import mandelboxFlow from "@app/main/flows/mandelbox"
import autoUpdateFlow from "@app/main/flows/autoupdate"
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

// Autoupdate flow
const update = autoUpdateFlow(fromTrigger(WhistTrigger.updateAvailable))

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

// Observable that fires when Fractal is ready to be launched
const launchTrigger = fromSignal(
  combineLatest({
    accessToken,
    configToken,
    isNewConfigToken,
    importCookiesFrom: fromTrigger(WhistTrigger.onboarded).pipe(
      startWith(undefined),
      map((payload) => payload?.importCookiesFrom)
    ),
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

createTrigger(WhistTrigger.authFlowSuccess, auth.success)
createTrigger(WhistTrigger.authFlowFailure, auth.failure)

createTrigger(WhistTrigger.updateDownloaded, update.downloaded)
createTrigger(WhistTrigger.downloadProgress, update.progress)

createTrigger(WhistTrigger.authRefreshSuccess, refreshAtEnd.success)

createTrigger(WhistTrigger.checkPaymentFlowSuccess, checkPayment.success)
createTrigger(WhistTrigger.checkPaymentFlowFailure, checkPayment.failure)

createTrigger(WhistTrigger.mandelboxFlowSuccess, mandelbox.success)
createTrigger(WhistTrigger.mandelboxFlowFailure, mandelbox.failure)
