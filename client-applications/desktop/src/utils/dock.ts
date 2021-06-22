import { app } from "electron"

export const showAppDock = () => {
  // On MacOS, the regular option creates an app with an app dock
  app?.setActivationPolicy?.("regular")
  // In case it's hidden, show the app dock
  app?.dock?.show().catch((err) => console.error(err))
}

export const hideAppDock = () => {
  // On MacOS, the accessory option removes the app dock
  app?.setActivationPolicy?.("accessory")
  // Hide the app dock just in case
  app?.dock?.hide()
}
