let shown = false

const element = (html: string) => {
  const template = document.createElement("template")
  html = html.trim()
  template.innerHTML = html
  return template.content.firstChild as HTMLElement
}

const createWarningNotification = ({
  text,
  duration,
}: {
  text: string
  duration?: number
}) => {
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
      background: #fee2e2;
      color: #c2410c;
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
            100% { opacity: 0; }
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
      createWarningNotification({
        text: "Cloud tabs are unavailable because Whist servers are full. Please try again in a few minutes!",
        duration: 7500,
      })
    }
    return true
  })
}

export { initNotificationListener }
