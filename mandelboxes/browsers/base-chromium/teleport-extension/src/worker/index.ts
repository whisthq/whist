/*
    The entry point to our service worker.
    A service worker is Javascript that runs in the background of Chrome.
    For more info, see https://developer.chrome.com/docs/workbox/service-worker-overview/
*/

import { initFileSyncHandler } from "./downloads"
import { initChromeWelcomeRedirect } from "./navigation"
import { initNativeHostIpc, initNativeHostDisconnectHandler } from "./ipc"
import { initCursorLockHandler } from "./cursor"
import { refreshExtension } from "./update"
import { initTabDetachSuppressor, initTabState } from "./tabs"
import { initLocationHandler } from "./geolocation"
import {
  initSocketioConnection,
  initActivateTabListener,
  initCloseTabListener,
  initCloudTabUpdatedListener,
  initCloudTabCreatedListener,
  initUpdateTabIDListener,
} from "./socketio"

console.log("Top of index.ts")

setTimeout(() => {
  console.log("inside the timeout")
  initTabState()

  const socket = initSocketioConnection()
  const nativeHostPort = initNativeHostIpc()
  
  // Disconnects the host native port on command
  initNativeHostDisconnectHandler(nativeHostPort)
  
  // If this is a new mandelbox, refresh the extension to get the latest version.
  refreshExtension(nativeHostPort)
  
  // Initialize the file upload/download handler
  initFileSyncHandler(nativeHostPort)
  
  // Enables relative mouse mode
  initCursorLockHandler(nativeHostPort)
  
  // Prevents tabs from being dragged out to new windows
  initTabDetachSuppressor()
  
  // Redirects the chrome://welcome page to our own Whist-branded page
  initChromeWelcomeRedirect()
  
  // Receive geolocation from extension host
  initLocationHandler(nativeHostPort)
  
  // Listen to the client for tab actions
  initActivateTabListener(socket)
  initCloseTabListener(socket)
  initUpdateTabIDListener(socket)
  initCloudTabUpdatedListener(socket)
  initCloudTabCreatedListener(socket)
}, 4000)

console.log("end of file")