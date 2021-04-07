import path from 'path'
import { app, BrowserWindow, BrowserWindowConstructorOptions } from 'electron'
import {
  WindowHashAuth,
  WindowHashUpdate,
  WindowHashAuthError,
  WindowHashProtocolError,
  WindowHashContainerError
} from '@app/utils/constants'

const buildRoot = app.isPackaged
  ? path.join(app.getAppPath(), 'build')
  : path.resolve('public')

const base = {
  webPreferences: { preload: path.join(buildRoot, 'preload.js') },
  resizable: false,
  titleBarStyle: 'hidden'
}

const screenWidth = {
  wXs: { width: 16 * 24 },
  wSm: { width: 16 * 32 },
  wMd: { width: 16 * 40 },
  wLg: { width: 16 * 56 },
  wXl: { width: 16 * 64 },
  w2Xl: { width: 16 * 80 },
  w3Xl: { width: 16 * 96 }
}

const screenHeight = {
  hXs: { height: 16 * 20 },
  hSm: { height: 16 * 32 },
  hMd: { height: 16 * 40 },
  hLg: { height: 16 * 56 },
  hXl: { height: 16 * 64 },
  h2Xl: { height: 16 * 80 },
  h3Xl: { height: 16 * 96 }
}

type CreateWindowFunction = (
  onReady?: (win: BrowserWindow) => any,
  onClose?: (win: BrowserWindow) => any
) => BrowserWindow

export const getWindows = () => BrowserWindow.getAllWindows()

export const closeWindows = () => {
  BrowserWindow.getAllWindows().forEach((win) => win.close())
}

export const createWindow = (
  show: string,
  options: Partial<BrowserWindowConstructorOptions>,
  onReady?: (win: BrowserWindow) => any,
  onClose?: (win: BrowserWindow) => any
) => {
  const win = new BrowserWindow({ ...options, show: false })

  const params = '?show=' + show

  if (app.isPackaged) {
    win.loadFile('build/index.html', { search: params })
  } else {
    win.loadURL('http://localhost:8080' + params)
    win.webContents.openDevTools({ mode: 'undocked' })
  }

  win.webContents.on('did-finish-load', () =>
    onReady != null ? onReady(win) : win.show()
  )
  win.on('close', () => onClose != null && onClose(win))

  return win
}

export const createAuthWindow: CreateWindowFunction = () =>
  createWindow(WindowHashAuth, {
    ...base,
    ...screenWidth.wSm,
    ...screenHeight.hMd
  } as BrowserWindowConstructorOptions)

export const createAuthErrorWindow: CreateWindowFunction = () =>
  createWindow(WindowHashAuthError, {
    ...base,
    ...screenWidth.wMd,
    ...screenHeight.hXs
  } as BrowserWindowConstructorOptions)

export const createContainerErrorWindow: CreateWindowFunction = () =>
  createWindow(WindowHashContainerError, {
    ...base,
    ...screenWidth.wMd,
    ...screenHeight.hXs
  } as BrowserWindowConstructorOptions)

export const createProtocolErrorWindow: CreateWindowFunction = () =>
  createWindow(WindowHashProtocolError, {
    ...base,
    ...screenWidth.wMd,
    ...screenHeight.hXs
  } as BrowserWindowConstructorOptions)

export const createUpdateWindow: CreateWindowFunction = () =>
  createWindow(WindowHashUpdate, {
    ...base,
    ...screenWidth.wSm,
    ...screenHeight.hMd
  } as BrowserWindowConstructorOptions)
