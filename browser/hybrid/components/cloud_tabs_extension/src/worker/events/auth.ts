import { merge, from, timer, interval, fromEventPattern } from "rxjs"
import { filter, share, switchMap, take, throttle, map } from "rxjs/operators"
import { NodeEventHandler } from "rxjs/internal/observable/fromEvent"

import { webpageLoaded } from "@app/worker/events/webRequest"
import { activatedFromSleep, networkOnline } from "@app/worker/events/idle"
import {
  authSuccess as _authSuccess,
  authFailure as _authFailure,
  authNetworkError as _authNetworkError,
  initWhistAuth,
} from "@app/worker/utils/auth"
import { authInfoCallbackRequest, parseAuthInfo } from "@app/@core-ts/auth"

import { AuthInfo } from "@app/@types/payload"
import { config } from "@app/constants/app"

const SEC_MS = 1000 // Number of ms in a second
const DAY_MS = 86400 * SEC_MS // Number of ms in a day

const googleAuth = fromEventPattern(
  (handler: NodeEventHandler) =>
    chrome.webRequest.onBeforeRequest.addListener(
      handler,
      {
        urls: [`${<string>config.AUTH0_REDIRECT_URL}*`],
      },
      ["blocking"]
    ),
  (handler: NodeEventHandler) =>
    chrome.webRequest.onBeforeRequest.removeListener(handler),
  (details: { url: string }) => details.url
).pipe(
  switchMap((url: string) => from(authInfoCallbackRequest(url))),
  map((response: any) => ({
    ...parseAuthInfo(response),
    refreshToken: response?.json.refresh_token,
    isFirstAuth: true,
  }))
)

const authInfo = merge(
  merge(
    webpageLoaded.pipe(take(1)),
    activatedFromSleep,
    networkOnline,
    timer(0, DAY_MS / 2)
  ).pipe(
    // We throttle by 10s so there's no race condition of two simultaneous auth refresh requests
    filter(() => navigator.onLine),
    throttle(() => interval(SEC_MS * 10)),
    switchMap(() => from(initWhistAuth()))
  ),
  googleAuth
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
