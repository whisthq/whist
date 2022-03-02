export const StateChannel = "MAIN_STATE_CHANNEL"
export const UIEventChannel = "UI_EVENT_CHANNEL"

export const ErrorIPC = [
  "Before you call useMainState(),",
  "an ipcRenderer must be attached to the renderer window object to",
  "communicate with the main process.",
  "\n\nYou need to attach it in an Electron preload.js file using",
  "contextBridge.exposeInMainWorld. You must explicity attach the 'on' and",
  "'send' methods for them to be exposed.",
].join(" ")
