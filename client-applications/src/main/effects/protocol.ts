/**
 * Copyright 2021 Fractal Computers, Inc., dba Whist
 * @file app.ts
 * @brief This file contains subscriptions to Observables related to protocol launching.
 */

import Sentry from "@sentry/electron"
import { map, startWith, withLatestFrom } from "rxjs/operators"

import { fromTrigger } from "@app/utils/flows"
import { protocolStreamInfo, childProcess } from "@app/utils/protocol"
import { createProtocolWindow } from "@app/utils/windows"
import { persistGet, persistSet } from "@app/utils/persist"
import { RESTORE_LAST_SESSION } from "@app/constants/store"
import { WhistTrigger } from "@app/constants/triggers"

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
          (payload: undefined | { importCookiesFrom: string | undefined }) =>
            payload?.importCookiesFrom
        )
      )
    )
  )
  .subscribe(
    ([info, importCookiesFrom]: [
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
        importCookiesFrom !== undefined ? 5000 : 0
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
