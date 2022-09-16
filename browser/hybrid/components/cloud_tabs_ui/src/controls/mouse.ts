import { getTabId } from "./session"

const initializeMouseEnterHandler = () => {
  const whistTag: any = document.querySelector("whist")
  whistTag.addEventListener("mouseenter", () => {
    ;(chrome as any).whist.broadcastWhistMessage(
      JSON.stringify({
        type: "MOUSE_ENTERED",
        value: {
          id: getTabId(),
        },
      })
    )
    console.log("mouse entered")
  })
}

export { initializeMouseEnterHandler }
