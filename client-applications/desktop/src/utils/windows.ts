import path from "path"
import { app, BrowserWindow, BrowserWindowConstructorOptions } from "electron"

const buildRoot = app.isPackaged
    ? path.join(app.getAppPath(), "build")
    : path.resolve("public")

const base = {
    webPreferences: { preload: path.join(buildRoot, "preload.js") },
    resizable: false,
    titleBarStyle: "hidden",
}

const wXs = { width: 16 * 24 }
const wSm = { width: 16 * 32 }
const wMd = { width: 16 * 40 }
const wLg = { width: 16 * 56 }
const wXl = { width: 16 * 64 }
const w2Xl = { width: 16 * 80 }
const w3Xl = { width: 16 * 96 }

const hXs = { height: 16 * 24 }
const hSm = { height: 16 * 32 }
const hMd = { height: 16 * 40 }
const hLg = { height: 16 * 56 }
const hXl = { height: 16 * 64 }
const h2Xl = { height: 16 * 80 }
const h3Xl = { height: 16 * 96 }

type CreateWindowFunction = (
    onReady?: (win: BrowserWindow) => any,
    onClose?: (win: BrowserWindow) => any
) => BrowserWindow

export const getWindows = () => BrowserWindow.getAllWindows()

export const createWindow = (
    options: Partial<BrowserWindowConstructorOptions>,
    onReady: (win: BrowserWindow) => any,
    onClose: (win: BrowserWindow) => any
) => {
    const win = new BrowserWindow({ ...options, show: false })

    if (app.isPackaged) {
        win.loadFile("build/index.html")
    } else {
        win.loadURL("http://localhost:8080")
        win.webContents.openDevTools({ mode: "undocked" })
    }

    win.webContents.on("did-finish-load", () => onReady(win))
    win.on("close", () => onClose(win))

    return win
}

export const createAuthWindow: CreateWindowFunction = (onReady, onClose) =>
    createWindow(
        { ...base, ...wSm, ...hMd } as BrowserWindowConstructorOptions,
        (win) => onReady && onReady(win),
        (win) => onClose && onClose(win)
    )

export const createErrorWindow: CreateWindowFunction = (onReady, onClose) =>
    createWindow(
        { ...base, ...wSm, ...hMd } as BrowserWindowConstructorOptions,
        (win) => onReady && onReady(win),
        (win) => onClose && onClose(win)
    )
