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

export const createAuthWindow = () => {
    const win = new BrowserWindow({
        ...base,
        ...wSm,
        ...hMd,
        show: false,
    } as BrowserWindowConstructorOptions)

    if (app.isPackaged) {
        win.loadFile("build/index.html")
    } else {
        win.loadURL("http://localhost:8080")
        win.webContents.openDevTools({ mode: "undocked" })
    }

    win.webContents.on("did-finish-load", () => win.show())

    return win
}

export const createErrorWindow = () => {
    const win = new BrowserWindow({
        ...base,
        ...wSm,
        ...hMd,
        show: false,
    } as BrowserWindowConstructorOptions)

    if (app.isPackaged) {
        win.loadFile("build/index.html")
    } else {
        win.loadURL("http://localhost:8080")
        win.webContents.openDevTools({ mode: "undocked" })
    }
}
