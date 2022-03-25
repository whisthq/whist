import {
  ContentScriptMessage,
  ContentScriptMessageType,
  NativeHostMessage,
  NativeHostMessageType,
} from "@app/constants/ipc"

const initCursorLockHandler = (nativeHostPort: chrome.runtime.Port) => {
  chrome.runtime.onMessage.addListener((msg: ContentScriptMessage) => {
    if (msg.type !== ContentScriptMessageType.POINTER_LOCK) {
      return
    }

    // Forward the message to the native host
    nativeHostPort.postMessage(<NativeHostMessage>{
      type: NativeHostMessageType.POINTER_LOCK,
      value: msg.value,
    })
  })
}

export { initCursorLockHandler }
