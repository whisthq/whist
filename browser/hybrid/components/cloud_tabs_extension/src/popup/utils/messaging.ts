import { PopupMessage, PopupMessageType } from "@app/@types/messaging"

const sendMessageToBackground = (
  type: PopupMessageType,
  value?: any,
  callback?: (response: any) => void
) => {
  chrome.runtime.sendMessage(
    <PopupMessage>{
      type,
      value,
    },
    callback
  )
}

export { sendMessageToBackground }
