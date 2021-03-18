const { contextBridge, ipcRenderer } = require("electron")

// Many of the properties of ipcRenderer are non-enumerable properties
// because they are inherited from EventEmitter. This requires them to
// be explicity named passed in the object below.

contextBridge.exposeInMainWorld(
    "ipcRenderer",
    {
        on: ipcRenderer.on,
        send: ipcRenderer.send
        removeListener: ipcRenderer.removeListener
    }
)
