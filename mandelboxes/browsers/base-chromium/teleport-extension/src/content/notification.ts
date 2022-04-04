import { injectResourceIntoDOM } from "@app/utils/resources"
import {
  ContentScriptMessage,
  ContentScriptMessageType,
} from "@app/constants/ipc"

const initFeatureWarnings = () => {
  injectResourceIntoDOM(document, "js/notification.js")
}

const initDownloadNotifications = () => {
  chrome.runtime.onMessage.addListener((msg: ContentScriptMessage) => {
    if (msg.type === ContentScriptMessageType.DOWNLOAD_COMPLETE)
      injectResourceIntoDOM(document, "js/download.js")
  })
}

export { initFeatureWarnings, initDownloadNotifications }
