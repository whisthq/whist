import { getTabId } from "./session"

const initializeDragHandler = () => {
  window.addEventListener("dragenter", () => {
    ;(chrome as any).whist.broadcastWhistMessage(
      JSON.stringify({
        type: "DRAG_ENTERED",
        value: {
          id: getTabId(),
        },
      })
    )
  })
}

export { initializeDragHandler }
