import { take, filter } from "rxjs/operators"
import { of } from "rxjs"
import Sentry from "@sentry/electron"
import isEmpty from "lodash.isempty"
import pickBy from "lodash.pickby"

import { withAppActivated } from "@app/main/utils/observables"
import { fromTrigger } from "@app/main/utils/flows"
import {
  WindowHashPayment,
  WindowHashOmnibar,
  WindowHashLoading,
  WindowHashOnboarding,
  WindowHashImport,
} from "@app/constants/windows"
import {
  createAuthWindow,
  createLoadingWindow,
  createSignoutWindow,
  createSpeedtestWindow,
  createPaymentWindow,
  createLicenseWindow,
  createImportWindow,
  createOnboardingWindow,
  createOmnibar,
} from "@app/main/utils/renderer"
import { persistGet } from "@app/main/utils/persist"
import { WhistTrigger } from "@app/constants/triggers"
import {
  CACHED_ACCESS_TOKEN,
  CACHED_CONFIG_TOKEN,
  CACHED_REFRESH_TOKEN,
  CACHED_USER_EMAIL,
  ONBOARDED,
} from "@app/constants/store"
import { networkAnalyze } from "@app/main/utils/networkAnalysis"
import { accessToken } from "@whist/core-ts"
import { destroyElectronWindow, hideElectronWindow } from "../utils/windows"

// Show the auth window if the user has not logged in yet
withAppActivated(
  of({
    accessToken: (persistGet(CACHED_ACCESS_TOKEN) ?? "") as string,
    refreshToken: (persistGet(CACHED_REFRESH_TOKEN) ?? "") as string,
    userEmail: (persistGet(CACHED_USER_EMAIL) ?? "") as string,
    configToken: (persistGet(CACHED_CONFIG_TOKEN) ?? "") as string,
  })
).subscribe((authCache) => {
  if (!isEmpty(pickBy(authCache, (x) => x === ""))) {
    createAuthWindow()
  }
})

withAppActivated(fromTrigger(WhistTrigger.mandelboxFlowStart)).subscribe(() => {
  if (persistGet(ONBOARDED) ?? false) {
    networkAnalyze()
    createLoadingWindow()
  }
})

withAppActivated(
  fromTrigger(WhistTrigger.protocolConnection).pipe(
    filter((connected: boolean) => connected)
  )
).subscribe(() => {
  createOmnibar()
})

withAppActivated(fromTrigger(WhistTrigger.stripeAuthRefresh)).subscribe(() => {
  destroyElectronWindow(WindowHashPayment)
})

withAppActivated(fromTrigger(WhistTrigger.showSignoutWindow)).subscribe(() => {
  hideElectronWindow(WindowHashOmnibar)
  createSignoutWindow()
})

withAppActivated(fromTrigger(WhistTrigger.showSpeedtestWindow)).subscribe(
  () => {
    hideElectronWindow(WindowHashOmnibar)
    createSpeedtestWindow()
  }
)

withAppActivated(fromTrigger(WhistTrigger.showImportWindow)).subscribe(() => {
  hideElectronWindow(WindowHashOmnibar)
  createImportWindow()
})

withAppActivated(fromTrigger(WhistTrigger.showLicenseWindow)).subscribe(() => {
  hideElectronWindow(WindowHashOmnibar)
  createLicenseWindow()
})

// eslint-disable-next-line @typescript-eslint/no-misused-promises
withAppActivated(fromTrigger(WhistTrigger.showPaymentWindow)).subscribe(() => {
  const accessToken = persistGet(CACHED_ACCESS_TOKEN) as string

  hideElectronWindow(WindowHashOmnibar)
  createPaymentWindow({
    accessToken,
  }).catch((err) => Sentry.captureException(err))
})

withAppActivated(fromTrigger(WhistTrigger.checkPaymentFlowFailure)).subscribe(
  ({ accessToken }: accessToken) => {
    createPaymentWindow({
      accessToken,
    }).catch((err) => Sentry.captureException(err))
  }
)

withAppActivated(fromTrigger(WhistTrigger.authFlowSuccess)).subscribe(() => {
  const onboarded = (persistGet(ONBOARDED) as boolean) ?? false
  if (!onboarded) {
    networkAnalyze()
    createOnboardingWindow()
  }
})

// When the protocol launches, destroy the loading window and onboarding window
// if they are open
withAppActivated(
  fromTrigger(WhistTrigger.protocolConnection).pipe(
    filter((connected) => connected)
  )
).subscribe(() => {
  destroyElectronWindow(WindowHashLoading)
  destroyElectronWindow(WindowHashOnboarding)
  destroyElectronWindow(WindowHashImport)
})

// When the protocol closes, also destroy the omnibar so it doesn't take up space
// in the background
fromTrigger(WhistTrigger.protocolClosed).subscribe(() => {
  destroyElectronWindow(WindowHashOmnibar)
})
