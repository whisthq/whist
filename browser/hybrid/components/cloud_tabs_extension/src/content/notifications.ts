import { CloudTabError, errorMessage } from "@app/constants/errors"

let shown = false

const element = (html: string) => {
  const template = document.createElement("template")
  html = html.trim()
  template.innerHTML = html
  return template.content.firstChild as HTMLElement
}

const createNotification = ({
  text,
  duration,
  type,
}: {
  text: string
  type?: string
  duration?: number
}) => {
  // Create the notification HTMLElement
  const notification = element(`<div class="whist-notification">${text}</div>`)

  notification.style.background = type === "warning" ? "#fee2e2" : "#bae6fd"
  notification.style.color = type === "warning" ? "#c2410c" : "#1d4ed8"

  // Inject the HTMLElement into the DOM
  shown = true
  document.body.appendChild(notification)
  // Fade out notification
  duration !== undefined &&
    setTimeout(() => {
      notification.style.animation = "fadeOut 1.5s"
      notification.style.animationFillMode = "forwards"
      shown = false
    }, duration)

  return notification
}

const initNotificationListener = () => {
  chrome.runtime.onMessage.addListener((message: any) => {
    if (shown || !Object.keys(errorMessage).includes(message.type)) return true

    const notification = createNotification(errorMessage[message.type])

    if (message.type === CloudTabError.AUTH_ERROR) {
      notification.addEventListener("click", () => {
        chrome.runtime.sendMessage({ type: "SHOW_LOGIN_PAGE" })
      })
    }

    if (message.type === CloudTabError.UPDATE_NEEDED) {
      notification.addEventListener("click", () => {
        chrome.runtime.sendMessage({ type: "SHOW_UPDATE_PAGE" })
      })
    }

    return true
  })
}

export { initNotificationListener }
