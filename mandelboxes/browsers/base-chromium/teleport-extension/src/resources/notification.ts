import { createNotification } from "@app/utils/notification"
import { URLS_THAT_NEED_CAMERA_MIC } from "@app/constants/urls"

const currentURL = document.URL

if (URLS_THAT_NEED_CAMERA_MIC.some((url) => currentURL.includes(url))) {
  createNotification(
    document,
    "Oops, you work faster than our engineers! This website may request access to your camera/mic, which we'll be supporting soon. In the meantime, please use another browser to access your camera/mic :)"
  )
}
