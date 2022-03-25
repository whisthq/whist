import {
  ContentScriptMessage,
  ContentScriptMessageType,
} from "@app/constants/ipc"

const initCursorLockDetection = () => {
  const onPointerLockChange = () => {
    const isPointerLocked = document.pointerLockElement !== null
    chrome.runtime.sendMessage(<ContentScriptMessage>{
      type: ContentScriptMessageType.POINTER_LOCK,
      value: isPointerLocked,
    })
  }

  document.addEventListener("pointerlockchange", onPointerLockChange)
}

export { initCursorLockDetection }
