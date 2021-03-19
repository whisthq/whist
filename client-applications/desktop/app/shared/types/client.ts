import { FractalNodeEnvironment } from "../types/config"

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
        const { app } = require("electron")
        if (app) {
            const appPath = app.getAppPath()
            if (process.env.NODE_ENV === FractalNodeEnvironment.DEVELOPMENT) {
                return path.join(appPath, "..")
            }
            return path.join(appPath, "../..")
        }
        return ""
    }
}
