import { createNotification } from "@app/utils/notification"
import { CAMERA_MIC_URLS } from "@app/constants/urls"

const currentURL = document.URL

if (CAMERA_MIC_URLS.some((url) => currentURL.includes(url))) {
  createNotification(
    document,
    "Sorry! Whist currently doesn't support websites that use your camera/mic. For now, please use your local browser or use the desktop app for this page :("
  )
}
