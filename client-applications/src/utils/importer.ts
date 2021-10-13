import { pickBy, keys } from "lodash"
import events from "events"

export const importEvent = new events.EventEmitter()

enum InstalledBrowser {
  BRAVE = "Brave",
  OPERA = "Opera",
  CHROME = "Chrome",
  FIREFOX = "Firefox",
  CHROMIUM = "Chromium",
}

const isBraveInstalled = () => {
  console.log("to do")
  return false
}

const isOperaInstalled = () => {
  console.log("to do")
  return false
}

const isChromeInstalled = () => {
  console.log("to do")
  return true
}

const isFirefoxInstalled = () => {
  console.log("to do")
  return false
}

const isChromiumInstalled = () => {
  console.log("to do")
  return false
}

const getInstalledBrowsers = () => {
  return keys(
    pickBy(
      {
        [InstalledBrowser.BRAVE]: isBraveInstalled(),
        [InstalledBrowser.OPERA]: isOperaInstalled(),
        [InstalledBrowser.CHROME]: isChromeInstalled(),
        [InstalledBrowser.FIREFOX]: isFirefoxInstalled(),
        [InstalledBrowser.CHROMIUM]: isChromiumInstalled(),
      },
      (v) => v
    )
  )
}

const aaronsCookieFunction = async (browser: InstalledBrowser) => {
  console.log("to do")
  importEvent.emit("cookies-imported")
}

export { InstalledBrowser, getInstalledBrowsers, aaronsCookieFunction }
