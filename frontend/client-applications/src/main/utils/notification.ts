import { Notification } from "electron"

const internetWarning = () =>
  new Notification({
    body: "Your Internet connection may be unstable",
    silent: true,
  })

const rebootWarning = () =>
  new Notification({
    body: "Whist unexpectedly disconnected and is rebooting",
    silent: true,
  })

const updateAvailableNotification = () =>
  new Notification({
    body: "An update is available! Whist is downloading it in the background.",
    silent: true,
  })

const updateDownloadedNotification = () =>
  new Notification({
    body: "Your update has been downloaded successfully. Whist will auto-update the next time it is closed.",
    silent: true,
  })

const startupNotification = () =>
  new Notification({
    body: "Whist is starting up",
    silent: true,
  })

export {
  internetWarning,
  rebootWarning,
  updateAvailableNotification,
  updateDownloadedNotification,
  startupNotification,
}
