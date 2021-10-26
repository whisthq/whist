import { merge, Observable, of, combineLatest } from "rxjs"
import { map, take, filter } from "rxjs/operators"
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
import TRIGGER from "@app/utils/triggers"
import {
  accessToken,
  refreshToken,
  userEmail,
  configToken,
} from "@app/utils/state"

// Autoupdate flow
const update = autoUpdateFlow(fromTrigger("updateAvailable"))

const auth = authFlow(
  merge(
    fromTrigger("authInfo"),
    combineLatest({
      accessToken,
      refreshToken,
      userEmail,
      configToken,
    }).pipe((take(1), filter((auth) => isEmpty(pickBy(auth, (x) => x === "")))))
  )
)

const onboarded = fromSignal(
  merge(
    merge(
      fromTrigger("cookiesImported"),
      fromTrigger("importerSubmitted").pipe(
        filter((payload) => payload.browser === "None")
      )
    ),
    of(persistGet("onboardingTypeformSubmitted", "data")).pipe(
      filter((onboarded) => onboarded as boolean)
    )
  ),
  fromTrigger(TRIGGER.authFlowSuccess)
)

// Unpack the access token to see if their payment is valid
const checkPayment = checkPaymentFlow(
  fromSignal(combineLatest({ accessToken }), onboarded)
)

// If the payment is invalid, they'll be redirect to the Stripe window. After that they'll
// get new auth credentials
const refreshAfterPaying = authRefreshFlow(
  fromSignal(combineLatest({ refreshToken }), fromTrigger("stripeAuthRefresh"))
)

// Observable that fires when Fractal is ready to be launched
const launchTrigger = fromSignal(
  combineLatest({ accessToken, configToken }).pipe(
    map((x: object) => ({
      ...x, // { accessToken, configToken }
      region: getRegionFromArgv(process.argv),
    })),
    take(1)
  ),
  merge(
    fromTrigger(TRIGGER.checkPaymentFlowSuccess),
    refreshAfterPaying.success
  )
)

// Mandelbox creation flow
const mandelbox = mandelboxFlow(launchTrigger)

// After the mandelbox flow is done, run the refresh flow so the tokens are being refreshed
// every time but don't impede startup time
const refreshAtEnd = authRefreshFlow(
  fromSignal(combineLatest({ refreshToken }), mandelbox.success)
)

createTrigger(TRIGGER.authFlowSuccess, auth.success)
createTrigger(TRIGGER.authFlowFailure, auth.failure)

createTrigger(TRIGGER.updateDownloaded, update.downloaded)
createTrigger(TRIGGER.downloadProgress, update.progress)

createTrigger(TRIGGER.authRefreshSuccess, refreshAtEnd.success)

createTrigger(TRIGGER.checkPaymentFlowSuccess, checkPayment.success)
createTrigger(TRIGGER.checkPaymentFlowFailure, checkPayment.failure)

createTrigger(TRIGGER.mandelboxFlowSuccess, mandelbox.success)
createTrigger(TRIGGER.mandelboxFlowFailure, mandelbox.failure)
