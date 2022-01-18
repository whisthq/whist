/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file app.ts
 * @brief This file contains subscriptions to Observables related to protocol launching.
 */

import Sentry from "@sentry/electron"
import { app } from "electron"
import { map, startWith, withLatestFrom } from "rxjs/operators"

import { fromTrigger } from "@app/main/utils/flows"
import {
  protocolStreamInfo,
  childProcess,
  protocolOpenUrl,
  protocolStreamKill,
} from "@app/main/utils/protocol"
import { createProtocolWindow } from "@app/main/utils/windows"
import { persistSet } from "@app/main/utils/persist"
import {
  RESTORE_LAST_SESSION,
  WHIST_IS_DEFAULT_BROWSER,
} from "@app/constants/store"
import { WhistTrigger } from "@app/constants/triggers"
import { logBase } from "@app/main/utils/logging"

// The current implementation of the protocol process shows its own loading
// screen while a mandelbox is created and configured. To do this, we need it
// the protocol to start and appear before its mandatory arguments are available.

// We solve this streaming the ip, secret_key, and ports info to the protocol
// they become available from when a successful mandelbox status response.

fromTrigger(WhistTrigger.mandelboxFlowSuccess)
  .pipe(
    withLatestFrom(
      fromTrigger(WhistTrigger.beginImport).pipe(
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

fromTrigger(WhistTrigger.restoreLastSession).subscribe(
  (body: { restore: boolean }) => {
    persistSet(RESTORE_LAST_SESSION, body.restore)
  }
)

fromTrigger(WhistTrigger.setDefaultBrowser).subscribe(
  (body: { default: boolean }) => {
    persistSet(WHIST_IS_DEFAULT_BROWSER, body.default)

    // if the value changed to true, then we need to set Whist as the default browser now.
    if (body.default) {
      app.setAsDefaultProtocolClient("http")
      app.setAsDefaultProtocolClient("https")
    } else {
      app.removeAsDefaultProtocolClient("http")
      app.removeAsDefaultProtocolClient("https")
    }
  }
)

fromTrigger(WhistTrigger.appReady).subscribe(() => {
  // Intercept URLs (Mac version)
  app.on("open-url", function (event, url: string) {
    event.preventDefault()
    protocolOpenUrl(url)
    logBase(`Captured url ${url} after setting Whist as default browser!\n`, {})
  })
})

fromTrigger(WhistTrigger.beginImport).subscribe(() => {
  protocolStreamKill()
})
