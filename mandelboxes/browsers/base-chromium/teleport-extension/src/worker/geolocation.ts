import { NativeHostMessage, NativeHostMessageType } from "@app/constants/ipc"

const initLocationSpoofer = (nativeHostPort: chrome.runtime.Port) => {
  // If this is a new mandelbox, refresh the extension to get the latest version.
  nativeHostPort.onMessage.addListener((msg: NativeHostMessage) => {
    if (msg.type === NativeHostMessageType.GEOLOCATION) {
      console.log("RECEIVED GEOLOCATION", msg.value)
    }
  })
}

export { initLocationSpoofer }
