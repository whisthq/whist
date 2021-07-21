/*
  Paths config
  This file exports the relevant paths for the protocol, app userdata, etc.
  These paths are dependent on operating system.
*/

const { app } = require("electron")
const path = require("path")

// Name of protocol executable
const getProtocolName = () => {
    if (process.platform === "win32") {
        return "Fractal.exe"
    } else if (process.platform === "darwin") {
        return "_Fractal"
    } else {
        return "Fractal"
    }
}

const getProtocolFolder = () => {
    if (app.isPackaged) {
        if (process.platform === "darwin") {
            return path.join(app.getAppPath(), "../..", "MacOS")
        } else {
            return path.join(app.getAppPath(), "../..", "protocol-build/client")
        }
    } else {
        return path.join(app.getAppPath(), "../../..", "protocol-build/client")
    }
}

const protocolName = getProtocolName()
const protocolFolder = getProtocolFolder()

// Log file names
const loggingFiles = {
    client: "client.log",
    protocol: "protocol.log",
}

// Root folder of built application
const buildRoot = app.isPackaged
    ? path.join(app.getAppPath(), "build")
    : path.resolve("public")

// Cache/persistence folder name
const userDataFolderNames = {
    development: "Electron",
    production: "fractal",
}

module.exports = {
    protocolName,
    protocolFolder,
    loggingFiles,
    userDataFolderNames,
    buildRoot,
}
