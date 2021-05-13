import { Subject } from "rxjs"

import { Menu, Tray, nativeImage } from "electron"
import { trayIconPath } from "@app/config/files"

let tray: Tray | null = null

const createNativeImage = () => {
  let image = nativeImage.createFromPath(trayIconPath)
  image = image.resize({ width: 16 })
  image.setTemplateImage(true)
  return image
}

export const signout = new Subject()
export const quit = new Subject()

export const createTray = (eventActionTypes: {
  signout: () => any
  quit: () => any
}) => {
  // We should only have one tray at any given time
  if (tray != null) {
    tray.destroy()
  }
  tray = new Tray(createNativeImage())
  const menu = Menu.buildFromTemplate([
    {
      label: "Sign out",
      click: () => {
        signout.next()
      },
    },
    {
      label: "Quit",
      click: () => {
        quit.next()
      },
    },
  ])
  tray.setContextMenu(menu)
}

export const doesTrayExist = () => {
  return tray != null && !tray.isDestroyed()
}