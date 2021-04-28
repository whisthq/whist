import { app, Menu, Tray, nativeImage } from "electron"
import path from "path"
let tray: Tray | null = null

export const createTray = (eventActionTypes: {
  signout: () => any
  quit: () => any
}) => {
  // we should only have one tray at any given time
  if (tray != null) {
    tray.destroy()
  }
  tray = new Tray(createNativeImage())
  const menu = Menu.buildFromTemplate([
    {
      label: "Sign out",
      click: () => {
        eventActionTypes.signout()
      },
    },
    {
      label: "Quit",
      click: () => {
        eventActionTypes.quit()
      },
    },
  ])
  tray.setContextMenu(menu)
}

export const doesTrayExist = () => {
  return tray != null && !tray.isDestroyed()
}

const getIcon = () => {
  let iconPath = ""
  if (app.isPackaged) {
    iconPath = path.join(app.getAppPath(), "../..")
  } else {
    iconPath = path.join(app.getAppPath(), "../../..")
  }

  if (process.platform === "win32") {
    return path.join(iconPath, "public/assets/images/trayIconPurple.ico")
  } else {
    return path.join(iconPath, "public/assets/images/trayIconBlackTemplate.png")
  }
}

const createNativeImage = () => {
  const path = getIcon()
  let image = nativeImage.createFromPath(path)
  image = image.resize({ width: 16 })
  image.setTemplateImage(true)
  return image
}
