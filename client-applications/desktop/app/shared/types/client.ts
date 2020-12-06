const homeDir = require("os").homedir()
const remote = require("electron").remote
    ? require("electron").remote.app.getAppPath()
    : ""

export enum OperatingSystem {
    WINDOWS = "win32",
    MAC = "darwin",
    LINUX = "linux",
}

export enum FractalAppName {
    CHROME = "Google Chrome",
}

export class FractalWindowsDirectory {
    static START_MENU = `${homeDir}\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\`

    static DESKTOP = `${homeDir}\\Desktop\\`

    static ROOT_DIRECTORY = `${remote}\\`
        .replace("\\resources\\app.asar", "")
        .replace("\\app\\", "\\")
}
