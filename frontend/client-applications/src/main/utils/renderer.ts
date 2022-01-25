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
  WindowHashLoading,
  WindowHashLicense,
  WindowHashImport,
  WindowHashOmnibar,
  WindowHashWelcome,
  WindowHashSupport,
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
      trafficLightPosition: { x: 22, y: 22 },
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
    ...height.md,
    hash: WindowHashOnboarding,
  })

const createUpdateWindow = () =>
  createElectronWindow({
    ...width.md,
    ...height.sm,
    hash: WindowHashUpdate,
  })

const createLoadingWindow = () =>
  createElectronWindow({
    ...width.md,
    ...height.sm,
    hash: WindowHashLoading,
  })

const createImportWindow = () =>
  createElectronWindow({
    ...width.md,
    ...height.md,
    hash: WindowHashImport,
  })

const createSupportWindow = () => {
  const { win } = createElectronWindow({
    ...width.xs,
    ...height.md,
    hash: WindowHashSupport,
    options: {
      alwaysOnTop: true,
    },
  })
  win.webContents.toggleDevTools()
}

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
      closable: false,
    },
    show: false,
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
  createLoadingWindow,
  createOnboardingWindow,
  createSignoutWindow,
  createUpdateWindow,
  createSupportWindow,
  createOmnibar,
}
