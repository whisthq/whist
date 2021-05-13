import { Menu, Tray, nativeImage } from "electron"
import { trayIconPath } from "@app/config/files"

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

const createNativeImage = () => {
  let image = nativeImage.createFromPath(trayIconPath)
  image = image.resize({ width: 16 })
  image.setTemplateImage(true)
  return image
}
