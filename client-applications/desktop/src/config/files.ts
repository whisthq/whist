import path from "path"
import { app } from "electron"

const deskTopFilePath = app.isPackaged
  ? path.join(app.getAppPath(), "../..")
  : path.join(app.getAppPath(), "../../..")

const trayIconMac = "public/assets/images/trayIconBlackTemplate.png"
const trayIconWindows = "public/assets/images/trayIconPurple.ico"
export const trayIconPath =
  process.platform === "win32"
    ? path.join(deskTopFilePath, trayIconWindows)
    : path.join(deskTopFilePath, trayIconMac)
