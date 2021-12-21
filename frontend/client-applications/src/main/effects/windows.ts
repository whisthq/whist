import { app, Notification } from "electron"
import { withLatestFrom, startWith, throttle, filter } from "rxjs/operators"
import { interval, of, merge } from "rxjs"
import Sentry from "@sentry/electron"
import isEmpty from "lodash.isempty"
import pickBy from "lodash.pickby"

import { destroyTray } from "@app/utils/tray"
import { logBase } from "@app/utils/logging"
import { withAppReady } from "@app/utils/observables"
import { fromTrigger, createTrigger } from "@app/utils/flows"
import { WindowHashProtocol } from "@app/constants/windows"
import {
  createProtocolWindow,
  createAuthWindow,
  createLoadingWindow,
} from "@app/utils/windows"
import { persistGet } from "@app/utils/persist"
import { internetWarning, rebootWarning } from "@app/utils/notification"
import { protocolStreamInfo, protocolStreamKill } from "@app/utils/protocol"
import { WhistTrigger } from "@app/constants/triggers"
import {
  CACHED_ACCESS_TOKEN,
  CACHED_CONFIG_TOKEN,
  CACHED_REFRESH_TOKEN,
  CACHED_USER_EMAIL,
  ONBOARDED,
} from "@app/constants/store"
import { networkAnalyze } from "@app/utils/networkAnalysis"

// Keeps track of how many times we've tried to relaunch the protocol
const MAX_RETRIES = 3
let protocolLaunchRetries = 0
// Notifications
let internetNotification: Notification | undefined
let rebootNotification: Notification | undefined

// Immediately initialize the protocol invisibly since it can take time to warm up
createProtocolWindow().catch((err) => Sentry.captureException(err))

fromTrigger(WhistTrigger.appReady).subscribe(() => {
  internetNotification = internetWarning()
  rebootNotification = rebootWarning()
})

const sleep = () => {
  logBase("Application quitting", {})
  destroyTray()
  protocolStreamKill()
  app?.quit()
}

const allWindowsClosed = fromTrigger(WhistTrigger.windowInfo).pipe(
  filter(
    (args: {
      crashed: boolean
      numberWindowsRemaining: number
      hash: string
      event: string
    }) => args.numberWindowsRemaining === 0
  )
)

allWindowsClosed.subscribe(
  (args: {
    crashed: boolean
    numberWindowsRemaining: number
    hash: string
    event: string
  }) => {
    // If they didn't crash out and didn't fill out the exit survey, show it to them
    if (
      args.hash !== WindowHashProtocol ||
      (args.hash === WindowHashProtocol && !args.crashed)
    ) {
      sleep()
    }
  }
)

fromTrigger(WhistTrigger.windowInfo)
  .pipe(
    withLatestFrom(
      fromTrigger(WhistTrigger.mandelboxFlowSuccess).pipe(startWith({}))
    )
  )
  .subscribe(
    ([args, info]: [
      {
        hash: string
        crashed: boolean
        event: string
      },
      any
    ]) => {
      if (
        args.hash === WindowHashProtocol &&
        args.crashed &&
        args.event === "close"
      ) {
        if (protocolLaunchRetries < MAX_RETRIES) {
          protocolLaunchRetries = protocolLaunchRetries + 1
          createProtocolWindow()
            .then(() => {
              protocolStreamInfo(info)
              rebootNotification?.show()
              setTimeout(() => {
                rebootNotification?.close()
              }, 6000)
            })
            .catch((err) => Sentry.captureException(err))
        } else {
          // If we've already tried several times to reconnect, just show the protocol error window
          createTrigger(WhistTrigger.protocolError, of(undefined))
        }
      }
    }
  )

fromTrigger(WhistTrigger.networkUnstable)
  .pipe(throttle(() => interval(30000))) // Throttle to 30s so we don't show too often
  .subscribe((unstable: boolean) => {
    if (unstable) internetNotification?.show()
    if (!unstable) internetNotification?.close()
  })

fromTrigger(WhistTrigger.appReady).subscribe(() => {
  const authCache = {
    accessToken: (persistGet(CACHED_ACCESS_TOKEN) ?? "") as string,
    refreshToken: (persistGet(CACHED_REFRESH_TOKEN) ?? "") as string,
    userEmail: (persistGet(CACHED_USER_EMAIL) ?? "") as string,
    configToken: (persistGet(CACHED_CONFIG_TOKEN) ?? "") as string,
  }

  if (!isEmpty(pickBy(authCache, (x) => x === ""))) {
    void app?.dock?.show()
    createAuthWindow()
  }
})

withAppReady(
  merge(
    fromTrigger(WhistTrigger.checkPaymentFlowSuccess),
    fromTrigger(WhistTrigger.stripeAuthRefresh)
  )
).subscribe(() => {
  if (persistGet(ONBOARDED) as boolean) {
    networkAnalyze()
    createLoadingWindow()
  }
})
