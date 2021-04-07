const { contextBridge, ipcRenderer } = require("electron")

// Many of the properties of ipcRenderer are non-enumerable properties
// because they are inherited from EventEmitter. This requires that they
// be wrapped in a new function object so they can correctly be passed
// to the renderer process.

contextBridge.exposeInMainWorld("ipcRenderer", {
    on: (...args) => ipcRenderer.on(...args),
    send: (...args) => ipcRenderer.send(...args),
    removeListener: (...args) => ipcRenderer.removeListener(...args),
})
