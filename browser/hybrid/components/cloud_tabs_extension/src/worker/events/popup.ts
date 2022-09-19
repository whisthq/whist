import { fromEventPattern } from "rxjs"
import { filter, share } from "rxjs/operators"

import { PopupMessageType } from "@app/@types/messaging"
import { NodeEventHandler } from "rxjs/internal/observable/fromEvent"

const popupMessage = fromEventPattern(
  (handler: NodeEventHandler) => {
    const wrapper = (request: any, sender: any, sendResponse: any) => {
      const event = { request, sender, sendResponse }
      handler(event)
      return true
    }
    chrome.runtime.onMessage.addListener(wrapper)
    return wrapper
  },
  (_handler: NodeEventHandler, wrapper: any) =>
    chrome.runtime.onMessage.removeListener(wrapper)
).pipe(share())

const popupToggled = popupMessage.pipe(
  filter((event: any) => event.request.type === PopupMessageType.STREAM_TAB)
)

const popupClicked = fromEventPattern(
  (handler: any) => chrome.browserAction.onClicked.addListener(handler),
  (handler: any) => chrome.browserAction.onClicked.removeListener(handler),
  (details: any) => details
)

const popupOpened = popupMessage.pipe(
  filter(
    (event: any) => event.request.type === PopupMessageType.FETCH_POPUP_DATA
  )
)

/* eslint-disable @typescript-eslint/strict-boolean-expressions */
const popupActivateCloudTab = popupToggled.pipe(
  filter((event: any) => event.request.value.enabled)
)

const popupDeactivateCloudTab = popupToggled.pipe(
  filter((event: any) => !event.request.value.enabled)
)

const popupUrlSaved = popupMessage.pipe(
  filter((event: any) => event.request.type === PopupMessageType.SAVE_URL),
  filter((event: any) => event.request.value.saved)
)

/* eslint-disable @typescript-eslint/strict-boolean-expressions */
const popupUrlUnsaved = popupMessage.pipe(
  filter((event: any) => event.request.type === PopupMessageType.SAVE_URL),
  filter((event: any) => !event.request.value.saved)
)

const popupOpenLogin = popupMessage.pipe(
  filter(
    (event: any) => event.request.type === PopupMessageType.OPEN_LOGIN_PAGE
  )
)

const popupInviteCode = popupMessage.pipe(
  filter((event: any) => event.request.type === PopupMessageType.INVITE_CODE)
)

export {
  popupClicked,
  popupActivateCloudTab,
  popupDeactivateCloudTab,
  popupOpened,
  popupUrlSaved,
  popupUrlUnsaved,
  popupOpenLogin,
  popupInviteCode,
}
