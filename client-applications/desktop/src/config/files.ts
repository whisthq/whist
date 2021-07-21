import path from "path"
import config from "@app/config/environment"
const { buildRoot } = config

const trayIconMac = "assets/images/trayIconBlackTemplate.png"
const trayIconWindows = "assets/images/trayIconPurple.ico"
export const trayIconPath =
    process.platform === "win32"
        ? path.join(buildRoot, trayIconWindows)
        : path.join(buildRoot, trayIconMac)
