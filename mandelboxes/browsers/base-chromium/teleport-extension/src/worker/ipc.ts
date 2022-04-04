import { NativeHostMessage, NativeHostMessageType } from "@app/constants/ipc"

const initNativeHostIpc = () => {
  const nativeHostPort = chrome.runtime.connectNative(
    "whist_teleport_extension_host"
  )

  return nativeHostPort
}

const initNativeHostDisconnectHandler = (nativeHostPort: chrome.runtime.Port) =>
  nativeHostPort.onMessage.addListener((msg: NativeHostMessage) => {
    if (msg.type === NativeHostMessageType.NATIVE_HOST_EXIT)
      nativeHostPort.disconnect()
  })

export { initNativeHostIpc, initNativeHostDisconnectHandler }
