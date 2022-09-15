import { PopupMessage, PopupMessageType } from "@app/@types/messaging"

const sendMessageToBackground = (
  type: PopupMessageType,
  value?: any,
  callback?: (response: any) => void
) => {
  const popupMessage = <PopupMessage>{ type, value }

  if (callback !== undefined) {
    chrome.runtime.sendMessage(popupMessage, callback)
  } else {
    chrome.runtime.sendMessage(popupMessage)
  }
}

export { sendMessageToBackground }
