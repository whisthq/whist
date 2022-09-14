import { css, element } from "./utils"

const createBackground = () => {
  const body = document.querySelector("body")
  let background = document.createElement("div")
  background.classList.add("background")

  css(background, {
    width: "100vw",
    height: "100vh",
    position: "absolute",
    top: 0,
    left: 0,
    "z-index": 1,
  })

  body?.appendChild(background)
  return background
}

const createLoadingMessage = (background: HTMLElement) => {
  let message = element(`
        <div style="position:absolute;left:50%;top:50%;transform:translate(-50%,-50%);">
          <svg style="height: 28px;top: 0.5px;position: relative; animation: rotate 2s linear infinite;" viewBox="0 0 50 50"><circle class="spinner spinner-dark" cx="25" cy="25" r="20" fill="none"></circle></svg>
        </div>
    `)

  if (message !== null) background.appendChild(message)
}

const removeBackgroundListener = (background?: HTMLElement) => {
  ;(chrome as any).whist.onMessage.addListener((message: string) => {
    const parsed = JSON.parse(message)
    if (parsed.type === "ACTIVATE_TAB" || parsed.type === "UPDATE_TAB") {
      // We don't remove the loading screen immediately because sometimes it takes time
      // for the handshake and the protocol to actually display
      setTimeout(() => {
        background?.remove()
      }, 7500)
    }
  })
}

const initializeLoadingScreen = () => {
  const background = createBackground()
  createLoadingMessage(background)
  removeBackgroundListener(background)
}

export { initializeLoadingScreen }
