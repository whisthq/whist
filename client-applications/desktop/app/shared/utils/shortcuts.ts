import { OperatingSystem } from "shared/enums/client"
import { FractalApp } from "shared/types/ui"

export const createShortcut = (app: FractalApp): boolean => {
    const os = require("os")
    const platform = os.platform()
    const app_url = `fractal://${app.app_id.toLowerCase().replace(/\s+/g, "-")}`

    if (platform === OperatingSystem.MAC) {
        console.log("mac shortcuts not yet implemented")
        return false
    } else if (platform === OperatingSystem.WINDOWS) {
        var path =
            require("electron").remote.app.getAppPath() +
            "\\node_modules\\@amdglobal\\create-desktop-shortcuts\\src\\windows.vbs"
        path = path.replace("app.asar", "app.asar.unpacked")

        const createDesktopShortcut = require("@amdglobal/create-desktop-shortcuts")
        const shortcutsCreated = createDesktopShortcut({
            windows: {
                filePath: app_url,
                name: app.app_id + " on Fractal",
                vbsPath: process.env.NODE_ENV === "development" ? "" : vbsPath,
            },
        })
        return shortcutsCreated
    } else {
        console.log(`no suitable os found, instead got ${platform}`)
        return false
    }
}

export const checkIfShortcutExists = (shortcut: string): boolean => {
    const fs = require("fs")
    const os = require("os")

    const platform = os.platform()
    const homeDir = require("os").homedir()
    const desktopPath = homeDir + "\\Desktop\\" + shortcut + ".lnk"
    const startMenuPath =
        homeDir +
        "\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\" +
        shortcut +
        ".lnk"

    try {
        if (platform === OperatingSystem.MAC) {
            console.log("mac shortcuts not yet implemented")
            return false
        } else if (platform === OperatingSystem.WINDOWS) {
            const exists =
                fs.existsSync(desktopPath) || fs.existsSync(startMenuPath)
            return exists
        } else {
            console.log(`no suitable os found, instead got ${platform}`)
            return false
        }
    } catch (err) {
        console.log(err)
        return false
    }
}
