import { element } from "./utils"
import { state } from "../shared/state"

let shown = false

const createNotification = ({
  text,
  duration,
  type,
}: {
  text: string
  duration?: number
  type?: string
}) => {
  // Create the notification HTMLElement
  let notification = element(`
    <div class="notification">
    ${
      type === "success"
        ? '<svg style="height: 18px;top: 0.5px;position: relative; color: #22c55e;" fill="none" viewBox="0 0 24 24" stroke-width="2" stroke="currentColor" aria-hidden="true"><path stroke-linecap="round" stroke-linejoin="round" d="M9 12l2 2 4-4m6 2a9 9 0 11-18 0 9 9 0 0118 0z"></path></svg>'
        : '<svg style="height: 18px;top: 0.5px;position: relative; animation: rotate 2s linear infinite;" viewBox="0 0 50 50"><circle class="spinner" cx="25" cy="25" r="20" fill="none" stroke-width="5" stroke="#22c55e"></circle></svg>'
    }
    <div style="padding-left:10px;">${text}</div>
    </div>
  `)

  // Inject the HTMLElement into the DOM
  ;(document.body || document.documentElement).appendChild(notification)
  shown = true
  // Fade out notification
  duration !== undefined &&
    setTimeout(() => {
      notification.style.animation = "fadeOut 1.5s"
      shown = false
    }, duration)

  return notification
}

export const createStartNotification = () => {
  let loaded = false

  ;(chrome as any).whist.onMessage.addListener((message: string) => {
    const parsed = JSON.parse(message)

    if (
      (parsed.type === "UPDATE_TAB" || parsed.type === "ACTIVATE_TAB") &&
      !shown &&
      !loaded
    ) {
      loaded = true
      createNotification({
        text: "Cloud tab activated",
        duration: 3500,
        type: "success",
      })
    }
  })
}

export const createDisconnectNotification = () => {
  let notification: HTMLElement | null = null
  const whistTag: any = document.querySelector("whist")

  setInterval(() => {
    if (!whistTag.isWhistConnected() && !shown && state.wasConnected) {
      notification = createNotification({
        text: "Connecting to cloud tab",
        type: "loading",
      })
    }
  }, 100)

  ;(chrome as any).whist.onMessage.addListener((message: string) => {
    const parsed = JSON.parse(message)

    if (parsed.type === "SOCKET_DISCONNECTED" && !shown) {
      notification = createNotification({
        text: "Connecting to cloud tab",
        type: "loading",
      })
    }

    if (
      (parsed.type === "UPDATE_TAB" ||
        parsed.type === "ACTIVATE_TAB" ||
        parsed.type === "SOCKET_CONNECTED") &&
      notification !== null
    ) {
      notification.style.animation = "fadeOut 1.5s"
      shown = false
    }
  })
}
