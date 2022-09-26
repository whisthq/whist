const INTERCOM_POPUP_WIDTH = 426
const INTERCOM_POPUP_HEIGHT = 626

export const createOrFocusHelpPopup = () => {
  chrome.tabs.query({ url: chrome.runtime.getURL("intercom.html") }, (tabs) => {
    if (tabs?.[0] !== undefined) {
      void chrome.windows.update(tabs?.[0].windowId, {
        drawAttention: true,
        focused: true,
      })
    } else {
      void chrome.windows.create({
        focused: true,
        url: chrome.runtime.getURL("intercom.html"),
        type: "popup",
        width: Math.min(INTERCOM_POPUP_WIDTH, screen.width),
        height: Math.min(INTERCOM_POPUP_HEIGHT, screen.height),
        left: Math.max(screen.width / 2 - INTERCOM_POPUP_WIDTH / 2, 10),
        top: Math.max(screen.height / 2 - INTERCOM_POPUP_HEIGHT / 2, 10),
      })
    }
  })
}
