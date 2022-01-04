/**
 * Copyright 2022 Whist Technologies, Inc.
 * @file app.ts
 * @brief This file contains subscriptions to Observables related to protocol launching.
 */

import Sentry from "@sentry/electron"
import { app } from "electron"
import { map, startWith, withLatestFrom } from "rxjs/operators"

import { fromTrigger } from "@app/utils/flows"
import {
  protocolStreamInfo,
  childProcess,
  ProtocolSendUrlToOpenInNewTab,
} from "@app/utils/protocol"
import { createProtocolWindow } from "@app/utils/windows"
import { persistGet, persistSet } from "@app/utils/persist"
import {
  RESTORE_LAST_SESSION,
  WHIST_IS_DEFAULT_BROWSER,
} from "@app/constants/store"
import { WhistTrigger } from "@app/constants/triggers"
import { logBase } from "@app/utils/logging"

// The current implementation of the protocol process shows its own loading
// screen while a mandelbox is created and configured. To do this, we need it
// the protocol to start and appear before its mandatory arguments are available.

// We solve this streaming the ip, secret_key, and ports info to the protocol
// they become available from when a successful mandelbox status response.

fromTrigger(WhistTrigger.mandelboxFlowSuccess)
  .pipe(
    withLatestFrom(
      fromTrigger(WhistTrigger.onboarded).pipe(
        startWith(undefined),
        map(
          (
            payload: undefined | { importBrowserDataFrom: string | undefined }
          ) => payload?.importBrowserDataFrom
        )
      )
    )
  )
  .subscribe(
    ([info, importBrowserDataFrom]: [
      {
        mandelboxIP: string
        mandelboxSecret: string
        mandelboxPorts: {
          port_32262: number
          port_32263: number
          port_32273: number
        }
      },
      string | undefined
    ]) => {
      setTimeout(
        () => {
          console.log("Connecting to", info)
          if (childProcess === undefined) {
            createProtocolWindow()
              .then(() => {
                protocolStreamInfo(info)
              })
              .catch((err) => Sentry.captureException(err))
          } else {
            protocolStreamInfo(info)
          }
        },
        importBrowserDataFrom !== undefined ? 5000 : 0
      )
    }
  )

fromTrigger("trayRestoreSessionAction").subscribe(() => {
  const restore = <boolean>persistGet(RESTORE_LAST_SESSION)
  if (restore !== undefined) {
    persistSet(RESTORE_LAST_SESSION, !restore)
  } else {
    persistSet(RESTORE_LAST_SESSION, true)
  }
})

fromTrigger("trayWhistIsDefaultBrowserAction").subscribe(() => {
  const whistDefaultBrowser =
    <boolean>persistGet(WHIST_IS_DEFAULT_BROWSER) ?? false
  persistSet(WHIST_IS_DEFAULT_BROWSER, !whistDefaultBrowser)
  // if the value changed to true, then we need to set Whist as the default browser now.
  if (!whistDefaultBrowser) {
    app.setAsDefaultProtocolClient("http")
    app.setAsDefaultProtocolClient("https")
  }
})

fromTrigger(WhistTrigger.appReady).subscribe(() => {
  // Check once at startup if we are supposed to have Whist as the default browser (might need to re-set Whist as default if we set another browser to default since the last execution)
  const whistShouldBeSetToDefault =
    <boolean>persistGet(WHIST_IS_DEFAULT_BROWSER) ?? false
  if (whistShouldBeSetToDefault) {
    app.setAsDefaultProtocolClient("http")
    app.setAsDefaultProtocolClient("https")
  }
  // Intercept URLs (Mac version)
  app.on("open-url", function (event, url: string) {
    event.preventDefault()
    ProtocolSendUrlToOpenInNewTab(url)
    logBase(`Captured url ${url} after setting Whist as default browser!\n`, {})
  })
})
