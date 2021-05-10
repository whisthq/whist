import { Menu, Tray, nativeImage } from "electron"
import { trayIconPath } from "@app/config/files"

// We create the tray here so that it persists throughout the application
let tray: Tray | null = null

export const createTray = (eventActionTypes: {
  signout: () => any
  quit: () => any
}) => {
  /*
    Description:
        Creates a tray for the application.
    Arguments:
        signout (function): function that triggers the signout action in the app
        quit (function): function that triggers the quit action in the app
    Returns:
        None
  */

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

// Process the image to the right size + set as template image (as a template image, Mac will automatically change the icon colors to match the mode)
const createNativeImage = () => {
  /*
    Description:
        Creates a tray icon based on the user's OS. Resizes the image and sets it as template image (as a template image, Mac will automatically change the icon colors to match the mode)
    Arguments:
        None
    Returns:
       (NativeImage): the tray icon
  */
  let image = nativeImage.createFromPath(trayIconPath)
  image = image.resize({ width: 16 })
  image.setTemplateImage(true)
  return image
}
