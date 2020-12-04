import { OperatingSystem } from "shared/types/client"
import { FractalApp } from "shared/types/ui"
import { FractalNodeEnvironment } from "shared/types/config"
import { debugLog } from "shared/utils/logging"

export const createShortcutName = (appName: string): string => {
    return `Fractalized ${appName}`
}

export const createShortcut = (app: FractalApp): boolean => {
    const os = require("os")
    const createDesktopShortcut = require("create-desktop-shortcuts")

    const platform: OperatingSystem = os.platform()
    const appURL = `fractal://${app.app_id.toLowerCase().replace(/\s+/g, "-")}`

    if (platform === OperatingSystem.MAC) {
        debugLog("Mac shortcuts not yet implemented")
        return false
    }
    if (platform === OperatingSystem.WINDOWS) {
        const vbsPath = `${require("electron")
            .remote.app.getAppPath()
            .replace(
                "app.asar",
                "app.asar.unpacked"
            )}\\node_modules\\create-desktop-shortcuts\\src\\windows.vbs`

        const shortcutsCreated = createDesktopShortcut({
            windows: {
                filePath: appURL,
                name: createShortcutName(app.app_id),
                vbsPath:
                    process.env.NODE_ENV === FractalNodeEnvironment.DEVELOPMENT
                        ? ""
                        : vbsPath,
            },
        })
        return shortcutsCreated
    }
    debugLog(`no suitable os found, instead got ${platform}`)
    return false
}

export const checkIfShortcutExists = (shortcut: string): boolean => {
    const fs = require("fs")
    const os = require("os")

    const platform = os.platform()
    const homeDir = require("os").homedir()
    const desktopPath = `${homeDir}\\Desktop\\${shortcut}.lnk`
    const startMenuPath = `${homeDir}\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\${shortcut}.lnk`

    try {
        if (platform === OperatingSystem.MAC) {
            debugLog("mac shortcuts not yet implemented")
            return false
        }
        if (platform === OperatingSystem.WINDOWS) {
            const exists =
                fs.existsSync(desktopPath) || fs.existsSync(startMenuPath)
            return exists
        }
        debugLog(`no suitable os found, instead got ${platform}`)
        return false
    } catch (err) {
        debugLog(err)
        return false
    }
}
