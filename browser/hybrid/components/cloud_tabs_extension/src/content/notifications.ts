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
  let background = "#bae6fd"
  let color = "#1d4ed8"

  if (type === "warning") {
    background = "#fee2e2"
    color = "#c2410c"
  }

  // Create the notification HTMLElement
  const notification = element(`
    <div style="
      animation: fadeIn 1.5s;
      animation-duration: 1.5s;
      font-family: BlinkMacSystemFont, Arial, Helvetica;
      z-index: 999999;
      padding: 15px 20px;
      position: fixed;
      top: 10px;
      right: 10px;
      border-radius: 5px;
      background: ${background};
      color: ${color};
      display: flex;
      align-items: center;
      backdrop-filter: blur(5px);
      font-size: 13px;
      font-weight: 500;"
    >
    <style>
        @keyframes fadeIn {
            0% { opacity: 0; }
            100% { opacity: 1; }
          }

        @keyframes fadeOut {
            0% { opacity: 1; }
            100% { opacity: 0; visibility: hidden; }
        }
      }
    </style>
    <div>${text}</div>
    </div>
  `)

  // Inject the HTMLElement into the DOM
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
    if (message.type === "SERVER_ERROR" && !shown) {
      shown = true
      createNotification({
        text: "Cloud tabs are unavailable because Whist servers are full. Please try again in a few minutes!",
        duration: 7500,
        type: "warning",
      })
    }

    if (message.type === "UPDATE_NEEDED" && !shown) {
      shown = true

      const notification = createNotification({
        text: `<span style="cursor: pointer;">🎉 &nbsp; An update is available! Click here to download the update and access cloud tabs.</span>`,
        duration: 10000,
      })

      notification.addEventListener("click", () => {
        chrome.runtime.sendMessage({ type: "SHOW_UPDATE_PAGE" })
      })
    }

    return true
  })
}

export { initNotificationListener }
