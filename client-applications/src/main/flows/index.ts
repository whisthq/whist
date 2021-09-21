import { merge, Observable, of } from "rxjs"
import { map, take, filter } from "rxjs/operators"
import { isEmpty, pickBy } from "lodash"

import authFlow, { authRefreshFlow } from "@app/main/flows/auth"
import checkPaymentFlow from "@app/main/flows/payment"
import mandelboxFlow from "@app/main/flows/mandelbox"
import autoUpdateFlow from "@app/main/flows/autoupdate"
import configFlow from "@app/main/flows/config"
import { fromTrigger, createTrigger } from "@app/utils/flows"
import { fromSignal } from "@app/utils/observables"
import { getRegionFromArgv } from "@app/utils/region"
import { AWSRegion } from "@app/@types/aws"
import { persistedAuth, persistGet } from "@app/utils/persist"
import TRIGGER from "@app/utils/triggers"

// Autoupdate flow
const update = autoUpdateFlow(fromTrigger("updateAvailable"))

// Auth flow
const authCache = {
  accessToken: (persistedAuth?.accessToken ?? "") as string,
  refreshToken: (persistedAuth?.refreshToken ?? "") as string,
  userEmail: (persistedAuth?.userEmail ?? "") as string,
  configToken: (persistedAuth?.configToken ?? "") as string,
}

if (isEmpty(pickBy(authCache, (x) => x === ""))) {
  const auth = authFlow(of(authCache))
  createTrigger(TRIGGER.authFlowSuccess, auth.success)
  createTrigger(TRIGGER.authFlowFailure, auth.failure)
} else {
  const auth = authFlow(fromTrigger("authInfo"))
  createTrigger(TRIGGER.authFlowSuccess, auth.success)
  createTrigger(TRIGGER.authFlowFailure, auth.failure)
}

const onboarded = fromSignal(
  merge(
    of(persistGet("onboardingTypeformSubmitted", "data")).pipe(
      filter((onboarded) => onboarded as boolean)
    ),
    fromTrigger("onboardingTypeformSubmitted")
  ),
  fromTrigger(TRIGGER.authFlowSuccess)
)

// Unpack the access token to see if their payment is valid
const checkPayment = checkPaymentFlow(
  fromSignal(fromTrigger(TRIGGER.authFlowSuccess), onboarded)
)

// If the payment is invalid, they'll be redirect to the Stripe window. After that they'll
// get new auth credentials
const refreshAfterPaying = authRefreshFlow(
  fromTrigger(TRIGGER.stripeAuthRefresh)
)

// If the payment is valid, get or generate the config token
const config = configFlow(
  merge(
    fromTrigger(TRIGGER.checkPaymentFlowSuccess),
    refreshAfterPaying.success
  )
)

// Observable that fires when Fractal is ready to be launched
const launchTrigger = fromTrigger(TRIGGER.configFlowSuccess).pipe(
  map((x: object) => ({
    ...x, // { accessToken, configToken }
    region: getRegionFromArgv(process.argv), // AWS region, if admins want to control the region
  })),
  take(1)
) as Observable<{
  accessToken: string
  configToken: string
  region?: AWSRegion
}>

// Mandelbox creation flow
const mandelbox = mandelboxFlow(launchTrigger)

// After the mandelbox flow is done, run the refresh flow so the tokens are being refreshed
// every time but don't impede startup time
const refreshAtEnd = authRefreshFlow(
  fromSignal(launchTrigger, mandelbox.success)
)

createTrigger(TRIGGER.updateDownloaded, update.downloaded)
createTrigger(TRIGGER.downloadProgress, update.progress)

createTrigger(TRIGGER.authRefreshSuccess, refreshAtEnd.success)

createTrigger(TRIGGER.checkPaymentFlowSuccess, checkPayment.success)
createTrigger(TRIGGER.checkPaymentFlowFailure, checkPayment.failure)

createTrigger(TRIGGER.mandelboxFlowSuccess, mandelbox.success)
createTrigger(TRIGGER.mandelboxFlowFailure, mandelbox.failure)

createTrigger(TRIGGER.configFlowSuccess, config.success)
