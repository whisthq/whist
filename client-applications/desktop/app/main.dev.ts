/* eslint global-require: off, no-console: off */

/**
 * This module executes inside of electron's main process. You can start
 * electron renderer process from here and communicate with the other processes
 * through IPC.
 *
 * When running `yarn build` or `yarn build-main`, this file is compiled to
 * `./app/main.prod.js` using webpack. This gives us some performance wins.
 */
import path from "path";
import { app, BrowserWindow } from "electron";
import { autoUpdater } from "electron-updater";
import log from "electron-log";
import MenuBuilder from "./menu";
import { dialog, Menu } from "electron";

var updating = false;
let mainWindow: BrowserWindow | null = null;

process.env.GOOGLE_API_KEY = "AIzaSyA2FUwAOXKqIWMqKN5DNPBUaqYMOWdBADQ";

if (process.env.NODE_ENV === "production") {
    const sourceMapSupport = require("source-map-support");
    sourceMapSupport.install();
}

if (
    process.env.NODE_ENV === "development" ||
    process.env.DEBUG_PROD === "true"
) {
    require("electron-debug")();
}

const installExtensions = async () => {
    const installer = require("electron-devtools-installer");
    const forceDownload = !!process.env.UPGRADE_EXTENSIONS;
    const extensions = ["REACT_DEVELOPER_TOOLS", "REDUX_DEVTOOLS"];

    return Promise.all(
        extensions.map((name) =>
            installer.default(installer[name], forceDownload)
        )
    ).catch(console.log);
};

const createWindow = async () => {
    if (
        process.env.NODE_ENV === "development" ||
        process.env.DEBUG_PROD === "true"
    ) {
        await installExtensions();
    }

    const os = require("os");
    if (os.platform() === "win32") {
        mainWindow = new BrowserWindow({
            show: false,
            width: 900,
            height: 600,
            frame: false,
            center: true,
            resizable: false,
            webPreferences: {
                nodeIntegration: true,
            },
        });
    } else if (os.platform() === "darwin") {
        mainWindow = new BrowserWindow({
            show: false,
            width: 900,
            height: 600,
            titleBarStyle: "hidden",
            center: true,
            resizable: false,
            maximizable: false,
            webPreferences: {
                nodeIntegration: true,
            },
        });
    } else if (os.platform() === "linux") {
        mainWindow = new BrowserWindow({
            show: false,
            width: 900,
            height: 600,
            titleBarStyle: "hidden",
            center: true,
            resizable: false,
            maximizable: false,
            webPreferences: {
                nodeIntegration: true,
            },
            icon: path.join(__dirname, "/build/icon.png"),
        });
    }

    console.log(path.join(__dirname, "/build/icon.png"));
    // mainWindow.webContents.openDevTools();
    mainWindow.loadURL(`file://${__dirname}/app.html`);

    // @TODO: Use 'ready-to-show' event
    //        https://github.com/electron/electron/blob/master/docs/api/browser-window.md#using-ready-to-show-event
    mainWindow.webContents.on("did-finish-load", () => {
        if (!mainWindow) {
            throw new Error('"mainWindow" is not defined');
        }
        if (process.env.START_MINIMIZED) {
            mainWindow.minimize();
        } else {
            mainWindow.show();
            mainWindow.focus();
            mainWindow.webContents.send("update", updating);
        }
    });

    mainWindow.on("closed", () => {
        mainWindow = null;
    });

    if (process.env.NODE_ENV === "development") {
        // Skip autoupdate check
    } else {
        autoUpdater.checkForUpdates();
    }
};

/**
 * Add event listeners...
 */

app.on("window-all-closed", () => {
    // Respect the OSX convention of having the application in memory even
    // after all windows have been closed
    if (process.platform !== "darwin") {
        app.quit();
    }
});

app.on("ready", createWindow);
app.on("activate", () => {
    // On macOS it's common to re-create a window in the app when the
    // dock icon is clicked and there are no other windows open.
    if (mainWindow === null) createWindow();
});

autoUpdater.autoDownload = false;

autoUpdater.on("update-available", () => {
    updating = true;
    mainWindow.webContents.send("update", updating);
    autoUpdater.downloadUpdate();
});

autoUpdater.on("update-not-available", () => {
    updating = false;
    mainWindow.webContents.send("update", updating);
});

autoUpdater.on("error", (ev, err) => {
    updating = false;
    mainWindow.webContents.send("error", err);
});

autoUpdater.on("download-progress", (progressObj) => {
    mainWindow.webContents.send("download-speed", progressObj.bytesPerSecond);
    mainWindow.webContents.send("percent", progressObj.percent);
    mainWindow.webContents.send("transferred", progressObj.transferred);
    mainWindow.webContents.send("total", progressObj.total);
});

autoUpdater.on("update-downloaded", () => {
    mainWindow.webContents.send("downloaded", true);
    autoUpdater.quitAndInstall();
    updating = false;
});
