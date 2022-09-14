import { fromEvent, fromEventPattern, from } from "rxjs"
import { switchMap, share, filter } from "rxjs/operators"
import { Socket } from "socket.io-client"

import { getTab, getActiveTab } from "@app/worker/utils/tabs"
import { socketConnected } from "@app/worker/events/socketio"

// All tab observables should emit a chrome.tabs.Tab
const tabCreated = fromEventPattern(
  (handler: any) => chrome.tabs.onCreated.addListener(handler),
  (handler: any) => chrome.tabs.onCreated.removeListener(handler),
  (tab: chrome.tabs.Tab) => tab
).pipe(share())

const tabActivated = fromEventPattern(
  (handler: any) => chrome.tabs.onActivated.addListener(handler),
  (handler: any) => chrome.tabs.onActivated.removeListener(handler),
  (activeInfo: { tabId: number }) => activeInfo.tabId
).pipe(
  switchMap((tabId: number) => from(getTab(tabId))),
  share()
)

const tabFocused = fromEventPattern(
  (handler: any) => chrome.windows.onFocusChanged.addListener(handler),
  (handler: any) => chrome.windows.onFocusChanged.removeListener(handler),
  (windowId: number) => windowId
).pipe(
  filter((windowId: number) => windowId > -1),
  switchMap((windowId: number) => from(getActiveTab(windowId))),
  share()
)

const tabRemoved = fromEventPattern(
  (handler: any) => chrome.tabs.onRemoved.addListener(handler),
  (handler: any) => chrome.tabs.onRemoved.removeListener(handler),
  (tabId: number, _removeInfo: object) => tabId
).pipe(share())

const tabUpdated = fromEventPattern(
  (handler: any) => chrome.tabs.onUpdated.addListener(handler),
  (handler: any) => chrome.tabs.onUpdated.removeListener(handler),
  (_tabId: number, _changeInfo: object, tab: chrome.tabs.Tab) => tab
).pipe(share())

const tabZoomed = fromEventPattern(
  (handler: any) => chrome.tabs.onZoomChange.addListener(handler),
  (handler: any) => chrome.tabs.onZoomChange.removeListener(handler),
  (zoomChangeInfo: object) => zoomChangeInfo
).pipe(share())

const cloudTabCreated = socketConnected
  .pipe(switchMap((s: Socket) => fromEvent(s, "tab-created")))
  .pipe(share())

const cloudTabUpdated = socketConnected
  .pipe(switchMap((s: Socket) => fromEvent(s, "tab-updated")))
  .pipe(share())

const cloudTabActivated = socketConnected
  .pipe(switchMap((s: Socket) => fromEvent(s, "tab-activated")))
  .pipe(share())

export {
  tabCreated,
  tabActivated,
  tabRemoved,
  tabUpdated,
  tabZoomed,
  cloudTabCreated,
  cloudTabUpdated,
  cloudTabActivated,
  tabFocused,
}
