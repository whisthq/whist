const homeDir = require("os").homedir()
const appPath = require("electron").remote
    ? require("electron").remote.app.getAppPath()
    : ""
const path = require("path")

export enum OperatingSystem {
    WINDOWS = "win32",
    MAC = "darwin",
    LINUX = "linux",
}

export enum FractalAppName {
    CHROME = "Google Chrome",
}

export class FractalDirectory {
    static WINDOWS_START_MENU = path.join(
        homeDir,
        "AppData/Roaming/Microsoft/Windows/Start Menu/Programs"
    )

    static MAC_APPLICATIONS = path.join(homeDir, "Applications")

    static DESKTOP = path.join(homeDir, "Desktop")

    static ROOT_DIRECTORY = appPath ? path.join(appPath, "../../..") : "" // undo resources/app.asar/app
}
