import { app, Menu, Tray, nativeTheme, nativeImage } from "electron"
// import path from "path"
let tray = null

export const createTray = (eventTrayActions: {
  signout: () => any
  quit: () => any
}) => {
  tray = new Tray(createNativeImage())
  tray.setPressedImage(createPressedImage())
  const menu = Menu.buildFromTemplate([
    {
      label: "Sign out",
      click: () => {
        eventTrayActions.signout()
      },
    },
    {
      label: "Quit",
      click: () => {
        eventTrayActions.quit()
      },
    },
  ])
  tray.setContextMenu(menu)
}

const getIcon = () => {
  if (process.platform === "win32") {
    return "/Users/janniezhong/Projects/fractal/fractal/client-applications/desktop/public/assets/images/trayIconPurple.ico"
  } else {
    if (!nativeTheme.shouldUseDarkColors) {
      return "/Users/janniezhong/Projects/fractal/fractal/client-applications/desktop/public/assets/images/trayIconBlack.png"
    } else {
      return "/Users/janniezhong/Projects/fractal/fractal/client-applications/desktop/public/assets/images/trayIconWhite.png"
    }
  }
}

const createNativeImage = () => {
  const path = getIcon()
  let image = nativeImage.createFromPath(path)
  console.log(image.isEmpty())
  image = image.resize({ width: 16 })
  return image
}

const createPressedImage = () => {
  // on Mac, pressing on the tray icon should invert the colors
  let image = nativeImage.createFromPath(
    "/Users/janniezhong/Projects/fractal/fractal/client-applications/desktop/public/assets/images/trayIconWhite.png"
  )
  image = image.resize({ width: 16 })
  return image
}
