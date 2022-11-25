import { fromEventPattern } from "rxjs"
import { filter } from "rxjs/operators"
import { NodeEventHandler } from "rxjs/internal/observable/fromEvent"

const webRequest = fromEventPattern(
  (handler: NodeEventHandler) =>
    chrome.webRequest.onBeforeRequest.addListener(handler, {
      urls: ["*://*/*"],
      types: ["main_frame"],
    }),
  (handler: NodeEventHandler) =>
    chrome.webRequest.onBeforeRequest.removeListener(handler),
  (response: any) => response
)

const webpageLoaded = fromEventPattern(
  (handler: any) => chrome.webNavigation.onCompleted.addListener(handler),
  (handler: any) => chrome.webNavigation.onCompleted.removeListener(handler),
  (details: any) => details
).pipe(filter((details: any) => details.url.startsWith("http")))

const webNavigationError = fromEventPattern(
  (handler: NodeEventHandler) =>
    chrome.webNavigation.onErrorOccurred.addListener(handler, {
      url: [{ schemes: ["http", "https"] }],
    }),
  (handler: NodeEventHandler) =>
    chrome.webNavigation.onErrorOccurred.removeListener(handler),
  (response: any) => response
)

export { webRequest, webpageLoaded, webNavigationError }
