const { app } = require('electron')
const path = require('path')

const getProtocolName = () => {
  if (process.platform === 'win32') {
    return 'Fractal.exe'
  } else if (process.platform === 'darwin') {
    return '_Fractal'
  } else {
    return 'Fractal'
  }
}

const getProtocolFolder = () => {
  if (app.isPackaged) {
    if (process.platform === 'darwin') {
      return path.join(app.getAppPath(), '../..', 'MacOS')
    } else {
      return path.join(app.getAppPath(), '../..', 'protocol-build/client')
    }
  } else {
    return path.join(app.getAppPath(), '../../..', 'protocol-build/client')
  }
}

const getBaseFilePath = () => {
  if (process.platform === 'win32') {
    return path.join(app.getPath('appData'), 'Fractal')
  } else {
    return path.join(app.getPath('home'), '.fractal')
  }
}

const baseFilePath = getBaseFilePath()
const protocolName = getProtocolName()
const protocolFolder = getProtocolFolder()

const loggingBaseFilePath =
  process.platform === 'win32'
    ? path.join(app.getPath('appData'), 'Fractal')
    : path.join(app.getPath('home'), '.fractal')

const buildRoot = 
  app.isPackaged
    ? path.join(app.getAppPath(), 'build')
    : path.resolve('public')

const userDataFolderNames = {
  development: 'Electron',
  production: 'Fractal'
}

module.exports = {
  baseFilePath,
  protocolName,
  protocolFolder,
  loggingBaseFilePath,
  userDataFolderNames,
  buildRoot
}