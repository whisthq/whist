import { NativeHostMessage, NativeHostMessageType } from "@app/constants/ipc"

const initNativeHostIpc = () =>
  chrome.runtime.connectNative("whist_teleport_extension_host")

const initNativeHostDisconnectHandler = (nativeHostPort: chrome.runtime.Port) =>
  nativeHostPort.onMessage.addListener((msg: NativeHostMessage) => {
    if (msg.type === NativeHostMessageType.NATIVE_HOST_EXIT)
      nativeHostPort.disconnect()
  })

export { initNativeHostDisconnectHandler, initNativeHostIpc }
