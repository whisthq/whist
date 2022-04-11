import events from "events"

import {
  width,
  height,
  WindowHashAuth,
  WindowHashPayment,
  WindowHashSignout,
  WindowHashOnboarding,
  WindowHashSpeedtest,
  WindowHashUpdate,
  WindowHashImportLoading,
  WindowHashLaunchLoading,
  WindowHashLicense,
  WindowHashImport,
  WindowHashOmnibar,
  WindowHashWelcome,
  WindowHashSupport,
  WindowHashRestoreTabs,
  WindowHashImportOnboarding,
} from "@app/constants/windows"
import {
  authPortalURL,
  authInfoParse,
  authInfoCallbackRequest,
  paymentPortalRequest,
  paymentPortalParse,
  accessToken,
} from "@whist/core-ts"
import { WhistCallbackUrls } from "@app/config/urls"
import { createElectronWindow } from "@app/main/utils/windows"

const window = new events.EventEmitter()

const createAuthWindow = () => {
  const { win, view } = createElectronWindow({
    ...width.md,
    ...height.lg,
    hash: WindowHashAuth,
    customURL: `${authPortalURL()}&connection=google-oauth2`,
    options: {
      fullscreenable: false,
      titleBarStyle: "customButtonsOnHover",
    },
  })

  /* eslint-disable @typescript-eslint/no-misused-promises */
  view?.webContents.session.webRequest.onBeforeRequest(
    {
      urls: [WhistCallbackUrls.authCallBack],
    },
    async ({ url }) => {
      const response = await authInfoCallbackRequest({ authCallbackURL: url })
      window.emit("auth-info", {
        ...authInfoParse(response),
        refreshToken: response?.json?.refresh_token,
      })
      return response
    }
  )

  return { win, view }
}

const createPaymentWindow = async (accessToken: accessToken) => {
  const response = await paymentPortalRequest(accessToken)
  const { paymentPortalURL } = paymentPortalParse(response)

  const { win, view } = createElectronWindow({
    ...width.xl,
    ...height.lg,
    hash: WindowHashPayment,
    customURL: paymentPortalURL ?? "",
  })

  /* eslint-disable @typescript-eslint/no-misused-promises */
  view?.webContents.session.webRequest.onBeforeRequest(
    {
      urls: [WhistCallbackUrls.paymentCallBack],
    },
    async ({ url }) => {
      // If checkout was successful or Stripe page was closed
      if (
        [
          "http://localhost/callback/payment?success=true",
          "http://localhost/callback/payment",
        ].includes(url)
      )
        window.emit("stripe-auth-refresh")
      // If the payment was not successful
      if (url === "http://localhost/callback/payment?success=false")
        window.emit("stripe-payment-error")
    }
  )

  return { win, view }
}

const createSpeedtestWindow = () =>
  createElectronWindow({
    ...width.lg,
    ...height.lg,
    hash: WindowHashSpeedtest,
    customURL: "https://speed.cloudflare.com/",
  })

const createLicenseWindow = () =>
  createElectronWindow({
    ...width.lg,
    ...height.lg,
    hash: WindowHashLicense,
    customURL:
      "https://whisthq.notion.site/Whist-Open-Source-Licenses-ea120824f105494bb721841e53a1d126",
  })

const createWelcomeWindow = () =>
  createElectronWindow({
    ...width.md,
    ...height.lg,
    hash: WindowHashWelcome,
    options: {
      titleBarStyle: "customButtonsOnHover",
    },
  })

const createErrorWindow = (hash: string) =>
  createElectronWindow({
    ...width.md,
    ...height.sm,
    hash,
    options: {
      alwaysOnTop: true,
    },
  })

const createSignoutWindow = () =>
  createElectronWindow({
    ...width.md,
    ...height.sm,
    hash: WindowHashSignout,
  })

const createOnboardingWindow = () =>
  createElectronWindow({
    ...width.md,
    ...height.lg,
    hash: WindowHashOnboarding,
    options: {
      fullscreenable: false,
      trafficLightPosition: { x: 12, y: 10 },
    },
  })

const createUpdateWindow = () =>
  createElectronWindow({
    ...width.md,
    ...height.sm,
    hash: WindowHashUpdate,
  })

const createLaunchLoadingWindow = () =>
  createElectronWindow({
    ...width.sm,
    ...height.md,
    hash: WindowHashLaunchLoading,
    options: {
      fullscreenable: false,
      titleBarStyle: "customButtonsOnHover",
    },
  })

const createImportLoadingWindow = () =>
  createElectronWindow({
    ...width.md,
    ...height.sm,
    hash: WindowHashImportLoading,
    options: {
      fullscreenable: false,
      titleBarStyle: "customButtonsOnHover",
    },
  })

const createImportWindow = () =>
  createElectronWindow({
    ...width.md,
    ...height.md,
    hash: WindowHashImport,
    options: {
      fullscreenable: false,
      titleBarStyle: "customButtonsOnHover",
    },
  })

const createImportOnboardingWindow = () =>
  createElectronWindow({
    ...width.md,
    ...height.lg,
    hash: WindowHashImportOnboarding,
    options: {
      fullscreenable: false,
      titleBarStyle: "customButtonsOnHover",
    },
  })

const createSupportWindow = () =>
  createElectronWindow({
    ...width.xs,
    ...height.md,
    hash: WindowHashSupport,
    options: {
      alwaysOnTop: true,
    },
  })

const createOmnibar = () =>
  createElectronWindow({
    ...width.md,
    ...height.sm,
    hash: WindowHashOmnibar,
    options: {
      alwaysOnTop: true,
      resizable: false,
      fullscreenable: false,
      minimizable: false,
      transparent: true,
      titleBarStyle: "default",
    },
    show: false,
  })

const createRestoreTabsWindow = () =>
  createElectronWindow({
    ...width.md,
    ...height.md,
    hash: WindowHashRestoreTabs,
  })

export {
  window,
  createAuthWindow,
  createPaymentWindow,
  createSpeedtestWindow,
  createLicenseWindow,
  createWelcomeWindow,
  createErrorWindow,
  createImportWindow,
  createImportLoadingWindow,
  createLaunchLoadingWindow,
  createOnboardingWindow,
  createSignoutWindow,
  createUpdateWindow,
  createSupportWindow,
  createRestoreTabsWindow,
  createOmnibar,
  createImportOnboardingWindow,
}
