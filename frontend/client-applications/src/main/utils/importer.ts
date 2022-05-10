import pickBy from "lodash.pickby"

import { InstalledBrowser } from "@app/constants/importer"
import {
  getCookieEncryptionKey,
  getCookiesFromFile,
  isBrowserInstalled,
  decryptCookies,
  getBookmarksFromFile,
  getExtensionIDs,
  getLocalStorageFromFiles,
  getExtensionSettingsFromFiles,
  getExtensionStateFromFiles,
  getPreferencesFromFile,
} from "@app/main/utils/crypto"


const getInstalledBrowsers = () => {
  return Object.keys(
    pickBy(
      {
        [InstalledBrowser.BRAVE]: isBrowserInstalled(InstalledBrowser.BRAVE),
        [InstalledBrowser.OPERA]: isBrowserInstalled(InstalledBrowser.OPERA),
        [InstalledBrowser.CHROME]: isBrowserInstalled(InstalledBrowser.CHROME),
        [InstalledBrowser.FIREFOX]:
          isBrowserInstalled(InstalledBrowser.FIREFOX) && false, // Firefox is not Chromium-based and not currently supported
        [InstalledBrowser.CHROMIUM]:
          isBrowserInstalled(InstalledBrowser.CHROMIUM) && false, // Chromium currently fails, so disabling
        [InstalledBrowser.EDGE]:
          isBrowserInstalled(InstalledBrowser.EDGE) && false, // Not tested
      },
      (v) => v
    )
  )
}

const getCookies = async (
  browser: InstalledBrowser
): Promise<string | undefined> => {
  if (browser === undefined) return undefined

  try {
    const encryptedCookies = await getCookiesFromFile(browser)
    const encryptKey = await getCookieEncryptionKey(browser)
    const cookies = await decryptCookies(encryptedCookies, encryptKey)

    return JSON.stringify(cookies)
  } catch (err) {
    return undefined
  }
}

const getBookmarks = async (
  browser: InstalledBrowser
): Promise<string | undefined> => {
  if (browser === undefined) return undefined
  const bookmarks = await getBookmarksFromFile(browser)
  if (bookmarks.length === 0) return undefined

  return bookmarks
}

const getExtensions = async (
  browser: InstalledBrowser
): Promise<string | undefined> => {
  // If no browser is requested or the browser is not recognized, don't run anything
  if (
    browser === undefined ||
    !Object.values(InstalledBrowser).includes(browser)
  )
    return undefined

  // For now we only want to get extensions for browsers that are compatible
  // with chrome extensions ie brave/chrome/chromium
  if (
    browser !== InstalledBrowser.CHROME &&
    browser !== InstalledBrowser.BRAVE &&
    browser !== InstalledBrowser.CHROMIUM
  ) {
    return undefined
  }

  const extensions = getExtensionIDs(browser)

  if (extensions.length === 0) return undefined

  return extensions
}

const getLocalStorage = async (
  browser: InstalledBrowser
): Promise<string | undefined> => {
  // If no browser is requested or the browser is not recognized, don't run anything
  if (
    browser === undefined ||
    !Object.values(InstalledBrowser).includes(browser)
  )
    return undefined

  // For now we only want to get local storage for browsers that are compatible
  // with chrome extensions ie brave/chrome/chromium
  if (
    browser !== InstalledBrowser.CHROME &&
    browser !== InstalledBrowser.BRAVE &&
    browser !== InstalledBrowser.CHROMIUM
  ) {
    return undefined
  }

  const localStorage = await getLocalStorageFromFiles(browser)

  if (localStorage.length === 0) return undefined

  return localStorage
}

/* eslint-disable no-unreachable */
const getPreferences = async (
  browser: InstalledBrowser
): Promise<string | undefined> => {
  // TODO: Make this not crash
  return undefined
  // If no browser is requested or the browser is not recognized, don't run anything
  if (
    browser === undefined ||
    !Object.values(InstalledBrowser).includes(browser)
  )
    return undefined

  // For now we only want to get extensions for browsers that are compatible
  // with chrome extensions ie brave/chrome/chromium
  if (
    browser !== InstalledBrowser.CHROME &&
    browser !== InstalledBrowser.BRAVE &&
    browser !== InstalledBrowser.CHROMIUM
  ) {
    return undefined
  }

  const preferences = getPreferencesFromFile(browser)
  if (preferences.length === 0) return undefined

  return preferences
}

const getExtensionState = async (
  browser: InstalledBrowser
): Promise<string | undefined> => {
  // If no browser is requested or the browser is not recognized, don't run anything
  if (
    browser === undefined ||
    !Object.values(InstalledBrowser).includes(browser)
  )
    return undefined

  // For now we only want to get local storage for browsers that are compatible
  // with chrome extensions ie brave/chrome/chromium
  if (
    browser !== InstalledBrowser.CHROME &&
    browser !== InstalledBrowser.BRAVE &&
    browser !== InstalledBrowser.CHROMIUM
  ) {
    return undefined
  }

  const extensionState = await getExtensionStateFromFiles(browser)

  if (extensionState.length === 0) return undefined

  return extensionState
}

const getExtensionSettings = async (
  browser: InstalledBrowser
): Promise<string | undefined> => {
  // If no browser is requested or the browser is not recognized, don't run anything
  if (
    browser === undefined ||
    !Object.values(InstalledBrowser).includes(browser)
  )
    return undefined

  // For now we only want to get local storage for browsers that are compatible
  // with chrome extensions ie brave/chrome/chromium
  if (
    browser !== InstalledBrowser.CHROME &&
    browser !== InstalledBrowser.BRAVE &&
    browser !== InstalledBrowser.CHROMIUM
  ) {
    return undefined
  }

  const extensionSettings = await getExtensionSettingsFromFiles(browser)

  if (extensionSettings.length === 0) return undefined

  return extensionSettings
}

export {
  InstalledBrowser,
  getInstalledBrowsers,
  getCookies,
  getBookmarks,
  getExtensions,
  getPreferences,
  getLocalStorage,
  getExtensionState,
  getExtensionSettings,
}
