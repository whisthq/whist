import { app, Menu, Tray, nativeTheme, nativeImage } from 'electron'
// import path from "path"
// import TrayIconDark from "@app/utils/assets/icon_bw2.svg"
// import TrayIconWhite from "@app/utils/assets/trayIconWhite.png"
// import TrayIconPurple from "assets/trayIconPurple.ico"
// import LogoPurple from "@app/renderer/assets/logoPurple.svg"

import { useMainState } from '@app/utils/ipc'
const [mainState, setMainState] = useMainState()

export const createTray = () => {
  const tray = new Tray(createNativeImage())
  if (process.platform === 'win32') {
    tray.on('right-click', () => {
      tray.popUpContextMenu()
    })
  }
  // tray.setPressedImage(TrayIconWhite)

  const menu = Menu.buildFromTemplate([
    {
      label: 'Sign out',
      click: () => {
        onSignout()
      }
    },
    {
      label: 'Quit',
      click: () => {
        app.quit()
      }
    }
  ])
  tray.setContextMenu(menu)
}

const getIcon = () => {
  if (process.platform === 'win32') {
    return './assets/trayIconPurple.ico'
  } else {
    if (!nativeTheme.shouldUseDarkColors) {
      return './assets/trayIconDark.png'
    } else {
      return './assets/trayIconWhite.png'
    }
  }
}

const createNativeImage = () => {
  // Since we never know where the app is installed,
  // we need to add the app base path to it.
  // const path = `${app.getAppPath()}/assets/trayIconDark.png`
  const path = getIcon()

  const image = nativeImage.createFromPath(path)
  // Marks the image as a template image.
  image.setTemplateImage(true)
  return image
}

const onSignout = () => {
  setMainState({
    signoutRequest: '1'
  })
}
