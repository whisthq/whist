import { NativeHostMessage, NativeHostMessageType } from "@app/constants/ipc"

const initNativeHostIpc = () => {
  const nativeHostPort = chrome.runtime.connectNative(
    "whist_teleport_extension_host"
  )

  nativeHostPort.onMessage.addListener((msg: NativeHostMessage) => {
    if (msg.type === NativeHostMessageType.NATIVE_HOST_EXIT) {
      nativeHostPort.disconnect()
    }
  })

  return nativeHostPort
}

export { initNativeHostIpc }
