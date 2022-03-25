import { NativeHostMessage, NativeHostMessageType } from "@app/constants/ipc"

/**
 * Unpacked extensions loaded with `--load-extension` are not automatically
 * updated. That is, the extension source may have changed, but a fresh chrome
 * run on a fresh mandelbox will persist the old extension source. Therefore,
 * when we're on a fresh mandelbox, we need to refresh the extension using
 * `chrome.runtime.reload`. Doing this unconditionally would be a bad idea,
 * since it would put us in an infinite loop. Instead, we use the native
 * messaging host to use the mandelbox filesystem to determine if this is a
 * fresh mandelbox, and trigger an extension update if so.
 *
 * Due to these conditions, any changes to the extension updating logic should
 * be carefully considered and backwards-compatible. Old mechanisms should
 * be grandfathered out to avoid breaking things for existing users.
 */

const refreshExtension = (nativeHostPort: chrome.runtime.Port) => {
  // If this is a new mandelbox, refresh the extension to get the latest version.
  nativeHostPort.onMessage.addListener((msg: NativeHostMessage) => {
    if (
      msg.type === NativeHostMessageType.NATIVE_HOST_UPDATE &&
      msg.value === true
    ) {
      chrome.runtime.reload()
    }
  })
}

export { refreshExtension }
