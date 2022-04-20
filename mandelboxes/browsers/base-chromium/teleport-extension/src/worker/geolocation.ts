import { NativeHostMessage, NativeHostMessageType } from "@app/constants/ipc"

const initLocationHandler = (nativeHostPort: chrome.runtime.Port) => {
  // If this is a new mandelbox, refresh the extension to get the latest version.
  nativeHostPort.onMessage.addListener((msg: NativeHostMessage) => {
    if (msg.type === NativeHostMessageType.GEOLOCATION) {
      console.log("Setting geolocation to", msg.value)

      chrome.storage.local.set({
        geolocation: msg.value,
      })
    }
  })
}

export { initLocationHandler }
