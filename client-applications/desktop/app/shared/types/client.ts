const homeDir = require("os").homedir()
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

    static getRootDirectory = () => {
        const remote = require("electron").remote
        if (remote && remote.app) {
            console.log("getting app path")
            const appPath = remote.app.getAppPath()
            console.log("done getting app path")
            return path.join(appPath, "../../..")
        }
        return ""
    }
}
