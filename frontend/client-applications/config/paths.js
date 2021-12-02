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
    return "Whist.exe"
  } else if (process.platform === "darwin") {
    return "WhistClient"
  } else {
    return "Whist"
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
    return path.resolve("protocol-build/client")
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

module.exports = {
  protocolName,
  protocolFolder,
  loggingFiles,
  buildRoot,
}
