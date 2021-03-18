import path from "path"
import url from "url"
import { app, BrowserWindow, dialog } from "electron"
import { windowThinSm } from "@app/utils/windows"

const buildRoot = app.isPackaged
    ? path.join(app.getAppPath(), "build")
    : path.resolve("public")

function createWindow(): void {
    // Create the browser window.
    const win = new BrowserWindow({
        ...windowThinSm,
        show: false,
        webPreferences: {
            preload: path.join(buildRoot, "preload.js"),
        },
    })

    if (process.env.NODE_ENV === "development") {
        win.loadURL("http://localhost:8080")
    } else {
        win.loadFile("build/index.html")
    }

    win.webContents.on("did-finish-load", function () {
        win.show()
    })

    win.webContents.openDevTools({ mode: "undocked" })
}

// This method will be called when Electron has finished
// initialization and is ready to create browser windows.
// Some APIs can only be used after this event occurs.
app.whenReady().then(createWindow)

// Quit when all windows are closed.
app.on("window-all-closed", () => {
    // On macOS it is common for applications and their menu bar
    // to stay active until the user quits explicitly with Cmd + Q
    if (process.platform !== "darwin") {
        app.quit()
    }
})

app.on("activate", () => {
    // On macOS it's common to re-create a window in the app when the
    // dock icon is clicked and there are no other windows open.
    if (BrowserWindow.getAllWindows().length === 0) {
        createWindow()
    }
})

// In this file you can include the rest of your app's specific main process
// code. You can also put them in separate files and require them
