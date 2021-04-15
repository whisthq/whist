const { contextBridge, ipcRenderer } = require("electron")

// Many of the properties of ipcRenderer are non-enumerable properties
// because they are inherited from EventEmitter. This requires that they
// be wrapped in a new function object so they can correctly be passed
// to the renderer process.
process.env.VERSION = "v2.1.0"

contextBridge.exposeInMainWorld("ipcRenderer", {
    on: (...args) => ipcRenderer.on(...args),
    send: (...args) => ipcRenderer.send(...args),
    removeListener: (...args) => ipcRenderer.removeListener(...args),
})

// We don't have access to the process object, so we must pass environment
// variables over to the window object here.


// The version number to be displayed on the renderer window.
contextBridge.exposeInMainWorld("VERSION", process.env.VERSION)
