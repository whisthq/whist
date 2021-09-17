import { Notification } from "electron"

const internetWarning = () =>
  new Notification({
    body: "Your Internet connection may be unstable",
    silent: true,
  })

const rebootWarning = () =>
  new Notification({
    body: "Fractal unexpectedly disconnected and is rebooting",
    silent: true,
  })

const updateNotification = () =>
  new Notification({
    body: "An update is available! Fractal is downloading it in the background.",
    silent: true,
  })

const startupNotification = () =>
  new Notification({
    body: "Fractal is starting up",
    silent: true,
  })

export {
  internetWarning,
  rebootWarning,
  updateNotification,
  startupNotification,
}
