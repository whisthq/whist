const { contextBridge, ipcRenderer } = require("electron")

contextBridge.exposeInMainWorld(
    "ipcRenderer",
    {
        on: ipcRenderer.on,
        send: ipcRenderer.send
    }
)
