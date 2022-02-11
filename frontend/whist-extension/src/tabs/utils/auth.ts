import { OPEN_GOOGLE_AUTH } from "@app/constants/messaging"

const openGoogleAuth = () => {
  chrome.runtime.sendMessage(OPEN_GOOGLE_AUTH)
}

export { openGoogleAuth }
