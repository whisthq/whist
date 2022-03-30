import { createNotification } from "@app/utils/notification"
import { URLS_THAT_NEED_CAMERA_MIC } from "@app/constants/urls"

const currentURL = document.URL

if (URLS_THAT_NEED_CAMERA_MIC.some((url) => currentURL.includes(url))) {
  createNotification(
    document,
    "Sorry! Whist currently doesn't support websites that use your camera/mic. For now, please use your local browser or use the desktop app for this page :("
  )
}
