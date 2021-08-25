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

export { internetWarning, rebootWarning }
