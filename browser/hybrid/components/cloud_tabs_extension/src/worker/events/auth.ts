import { merge, from, timer } from "rxjs"
import { filter, share, switchMap, take } from "rxjs/operators"

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

const DAY_MS = 86400000 // The number of milliseconds in a day

const authInfo = merge(
  webpageLoaded.pipe(
    take(1),
    switchMap(() => from(initWhistAuth()))
  ),
  activatedFromSleep.pipe(switchMap(() => from(initWhistAuth()))),
  timer(0, DAY_MS / 2).pipe(switchMap(() => from(initWhistAuth()))),
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
