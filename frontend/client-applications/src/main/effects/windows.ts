import { BrowserWindow } from "electron"
import { of } from "rxjs"
import { filter, withLatestFrom, map, mapTo, startWith } from "rxjs/operators"
import Sentry from "@sentry/electron"
import isEmpty from "lodash.isempty"
import pickBy from "lodash.pickby"

import {
  withAppActivated,
  untilUpdateAvailable,
} from "@app/main/utils/observables"
import { fromTrigger } from "@app/main/utils/flows"
import {
  WindowHashPayment,
  WindowHashOmnibar,
  WindowHashLaunchLoading,
  WindowHashImportLoading,
  WindowHashOnboarding,
  WindowHashImport,
  WindowHashAuth,
  WindowHashWelcome,
  WindowHashSupport,
  WindowHashRestoreTabs,
  WindowHashImportOnboarding,
} from "@app/constants/windows"
import {
  createAuthWindow,
  createLaunchLoadingWindow,
  createSignoutWindow,
  createSpeedtestWindow,
  createPaymentWindow,
  createLicenseWindow,
  createImportWindow,
  createOnboardingWindow,
  createWelcomeWindow,
  createOmnibar,
  createSupportWindow,
  createRestoreTabsWindow,
  createImportOnboardingWindow,
  createImportLoadingWindow,
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
import { NO_PAYMENT_ERROR } from "@app/constants/error"
import { networkAnalyze } from "@app/main/utils/networkAnalysis"
import {
  destroyElectronWindow,
  hideElectronWindow,
  getElectronWindowHash,
} from "@app/main/utils/windows"

// Show the welcome window if the user has not logged in yet
untilUpdateAvailable(
  withAppActivated(
    of({
      accessToken: (persistGet(CACHED_ACCESS_TOKEN) ?? "") as string,
      refreshToken: (persistGet(CACHED_REFRESH_TOKEN) ?? "") as string,
      userEmail: (persistGet(CACHED_USER_EMAIL) ?? "") as string,
      configToken: (persistGet(CACHED_CONFIG_TOKEN) ?? "") as string,
    })
  )
).subscribe((authCache) => {
  if (!isEmpty(pickBy(authCache, (x) => x === ""))) {
    createWelcomeWindow()
  }
})

// Show the auth window after the user passes the welcome window
withAppActivated(fromTrigger(WhistTrigger.showAuthWindow)).subscribe(() => {
  createAuthWindow()
  destroyElectronWindow(WindowHashWelcome)
})

// Show the loading window whenever we start the mandelbox flow
untilUpdateAvailable(
  withAppActivated(
    fromTrigger(WhistTrigger.mandelboxFlowStart).pipe(
      withLatestFrom(
        fromTrigger(WhistTrigger.beginImport).pipe(
          mapTo(true),
          startWith(false)
        ),
        fromTrigger(WhistTrigger.protocolConnection)
      ),
      map((x) => ({ import: x[1], connected: x[2] }))
    )
  )
).subscribe((args: { import: boolean; connected: boolean }) => {
  if (!args.import) networkAnalyze()

  console.log("PROTOCOL CONNECTION IS", args.connected)

  args.connected ? createImportLoadingWindow() : createLaunchLoadingWindow()

  destroyElectronWindow(WindowHashPayment)
  destroyElectronWindow(WindowHashImport)
  destroyElectronWindow(WindowHashImportOnboarding)
  destroyElectronWindow(WindowHashOnboarding)
})

withAppActivated(fromTrigger(WhistTrigger.stripeAuthRefresh)).subscribe(() => {
  const onboarded = (persistGet(ONBOARDED) as boolean) ?? false
  if (!onboarded) createImportOnboardingWindow()
  destroyElectronWindow(WindowHashPayment)
})

withAppActivated(
  fromTrigger(WhistTrigger.protocolConnection).pipe(
    filter((connected: boolean) => connected)
  )
).subscribe(() => {
  const { win } = createOmnibar()
  win?.on("blur", () => {
    hideElectronWindow(WindowHashOmnibar)
  })
})

withAppActivated(fromTrigger(WhistTrigger.showSignoutWindow)).subscribe(() => {
  hideElectronWindow(WindowHashOmnibar)
  createSignoutWindow()

  BrowserWindow.getAllWindows().forEach((win) => {
    const hash = getElectronWindowHash(win)
    if ((hash?.includes("error") ?? false) && hash !== undefined)
      destroyElectronWindow(hash)
  })
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

withAppActivated(fromTrigger(WhistTrigger.showPaymentWindow)).subscribe(() => {
  const accessToken = persistGet(CACHED_ACCESS_TOKEN) as string

  hideElectronWindow(WindowHashOmnibar)
  createPaymentWindow({
    accessToken,
  })
    .then(() => {
      destroyElectronWindow(NO_PAYMENT_ERROR)
      destroyElectronWindow(WindowHashOnboarding)
    })
    .catch((err) => Sentry.captureException(err))
})

untilUpdateAvailable(
  withAppActivated(fromTrigger(WhistTrigger.authFlowSuccess))
).subscribe(() => {
  const onboarded = (persistGet(ONBOARDED) as boolean) ?? false
  if (!onboarded) {
    createOnboardingWindow()
    destroyElectronWindow(WindowHashAuth)
  }
})

withAppActivated(fromTrigger(WhistTrigger.showSupportWindow)).subscribe(() => {
  hideElectronWindow(WindowHashOmnibar)
  createSupportWindow()
})

withAppActivated(fromTrigger(WhistTrigger.showRestoreTabsWindow)).subscribe(
  () => {
    hideElectronWindow(WindowHashOmnibar)
    createRestoreTabsWindow()
  }
)

withAppActivated(fromTrigger(WhistTrigger.closeSupportWindow)).subscribe(() => {
  destroyElectronWindow(WindowHashSupport)
})

withAppActivated(fromTrigger(WhistTrigger.importTabs)).subscribe(() => {
  destroyElectronWindow(WindowHashRestoreTabs)
})
// When the protocol launches, destroy the loading window and onboarding window
// if they are open
withAppActivated(
  fromTrigger(WhistTrigger.protocolConnection).pipe(
    filter((connected) => connected)
  )
).subscribe(() => {
  destroyElectronWindow(WindowHashLaunchLoading)
  destroyElectronWindow(WindowHashImportLoading)
  destroyElectronWindow(WindowHashOnboarding)
  destroyElectronWindow(WindowHashImport)
})
