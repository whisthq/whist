import { merge, from, timer, interval } from "rxjs"
import { filter, share, switchMap, take, throttle } from "rxjs/operators"

import { webpageLoaded } from "@app/worker/events/webRequest"
import { activatedFromSleep } from "@app/worker/events/idle"
import {
  authSuccess as _authSuccess,
  authFailure as _authFailure,
  authNetworkError as _authNetworkError,
  initGoogleAuthHandler,
  initWhistAuth,
} from "@app/worker/utils/auth"

import { AuthInfo } from "@app/@types/payload"

const SEC_MS = 1000 // Number of ms in a second
const DAY_MS = 86400 * SEC_MS // Number of ms in a day

const authInfo = merge(
  merge(
    webpageLoaded.pipe(take(1)),
    activatedFromSleep,
    timer(0, DAY_MS / 2)
  ).pipe(
    // We throttle by 10s so there's no race condition of two simultaneous auth refresh requests
    throttle(() => interval(SEC_MS * 10)),
    switchMap(() => from(initWhistAuth()))
  ),
  from(initGoogleAuthHandler())
).pipe(share())

const authSuccess = authInfo.pipe(
  filter((auth: AuthInfo) => _authSuccess(auth))
)
const authFailure = authInfo.pipe(
  filter((auth: AuthInfo) => _authFailure(auth))
)
const authNetworkError = authInfo.pipe(
  filter((auth: AuthInfo) => _authNetworkError(auth))
)

export { authInfo, authSuccess, authFailure, authNetworkError }
