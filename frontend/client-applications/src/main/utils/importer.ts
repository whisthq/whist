import pickBy from "lodash.pickby"
import keytar from "keytar"
import dbus from "dbus-next"
import fs from "fs"
import tmp from "tmp"
import { homedir } from "os"
import path, { dirname } from "path"
// import Database from "better-sqlite3"
import knex from "knex"
import crypto from "crypto"

import {
  ALGORITHM_PLAIN,
  BraveLinuxDefaultDir,
  BraveOSXDefaultDir,
  BusSecretName,
  BusSecretPath,
  ChromeLinuxDefaultDir,
  ChromeOSXDefaultDir,
  ChromiumLinuxDefaultDir,
  ChromiumOSXDefaultDir,
  EdgeLinuxDefaultDir,
  EdgeOSXDefaultDir,
  OperaLinuxDefaultDir,
  OperaOSXDefaultDir,
  SecretServiceName,
  InstalledBrowser,
} from "@app/constants/importer"

const DEFAULT_ENCRYPTION_KEY = "peanuts"

interface Cookie {
  [key: string]: Buffer | string | number
}

const getBrowserDefaultDirectory = (browser: InstalledBrowser): string[] => {
  switch (process.platform) {
    case "darwin": {
      switch (browser) {
        case InstalledBrowser.CHROME: {
          return ChromeOSXDefaultDir
        }
        case InstalledBrowser.OPERA: {
          return OperaOSXDefaultDir
        }
        case InstalledBrowser.EDGE: {
          return EdgeOSXDefaultDir
        }
        case InstalledBrowser.CHROMIUM: {
          return ChromiumOSXDefaultDir
        }
        case InstalledBrowser.BRAVE: {
          return BraveOSXDefaultDir
        }
        default: {
          return []
        }
      }
    }
    case "linux": {
      switch (browser) {
        case InstalledBrowser.CHROME: {
          return ChromeLinuxDefaultDir
        }
        case InstalledBrowser.OPERA: {
          return OperaLinuxDefaultDir
        }
        case InstalledBrowser.EDGE: {
          return EdgeLinuxDefaultDir
        }
        case InstalledBrowser.CHROMIUM: {
          return ChromiumLinuxDefaultDir
        }
        case InstalledBrowser.BRAVE: {
          return BraveLinuxDefaultDir
        }
        default: {
          return []
        }
      }
    }
    default: {
      return []
    }
  }
}

const getCookieFilePath = (browser: InstalledBrowser): string[] => {
  const browserDirectories = getBrowserDefaultDirectory(browser)
  return browserDirectories.map((dir) => path.join(dir, "Cookies"))
}

const getBookmarkFilePath = (browser: InstalledBrowser): string[] => {
  const browserDirectories = getBrowserDefaultDirectory(browser)
  return browserDirectories.map((dir) => path.join(dir, "Bookmarks"))
}

const getExtensionDir = (browser: InstalledBrowser): string[] => {
  const browserDirectories = getBrowserDefaultDirectory(browser)
  return browserDirectories.map((dir) => path.join(dir, "Extensions"))
}

const getOsCryptName = (browser: InstalledBrowser): string => {
  switch (browser) {
    case InstalledBrowser.CHROME: {
      return "chrome"
    }
    case InstalledBrowser.OPERA: {
      return "chromium"
    }
    case InstalledBrowser.EDGE: {
      return "chromium"
    }
    case InstalledBrowser.CHROMIUM: {
      return "chromium"
    }
    case InstalledBrowser.BRAVE: {
      return "brave"
    }
  }
  return ""
}

const getKeyUser = (browser: InstalledBrowser): string => {
  switch (browser) {
    case InstalledBrowser.CHROME: {
      return "Chrome"
    }
    case InstalledBrowser.OPERA: {
      return "Opera"
    }
    case InstalledBrowser.EDGE: {
      return "Microsoft Edge"
    }
    case InstalledBrowser.CHROMIUM: {
      return "Chromium"
    }
    case InstalledBrowser.BRAVE: {
      return "Brave"
    }
    default: {
      return ""
    }
  }
}

const getKeyService = (browser: InstalledBrowser): string => {
  return `${getKeyUser(browser)} Safe Storage`
}

const getLinuxCookieEncryptionKey = async (
  browser: InstalledBrowser
): Promise<string> => {
  const bus = dbus.sessionBus()

  const osCryptName = getOsCryptName(browser)

  const proxyToSecret = await bus.getProxyObject(BusSecretName, BusSecretPath)

  const secretService = await proxyToSecret.getInterface(SecretServiceName)

  // https://github.com/borisbabic/browser_cookie3/blob/master/__init__.py#L120
  let [secretLocations] = await secretService.SearchItems({
    "xdg:schema": "chrome_libsecret_os_crypt_password_v2",
    application: osCryptName.toLowerCase(),
  })

  if (secretLocations.length === 0) {
    ;[secretLocations] = await secretService.SearchItems({
      "xdg:schema": "chrome_libsecret_os_crypt_password_v1",
      application: osCryptName.toLowerCase(),
    })
  }

  if (secretLocations.length > 0) {
    const variant = new dbus.Variant("s", "")

    // TODO (aaron): add different encryptions for getting password
    const [, sessionPath] = await secretService.OpenSession(
      ALGORITHM_PLAIN,
      variant
    )

    const secretObj: { [key: string]: Array<string | any> } =
      await secretService.GetSecrets(secretLocations, sessionPath)

    bus.disconnect()

    // secret object should contain the path the secret as the key, and the session path, empty buffer, password buffer, and encryption type as the value
    if (
      Object.keys(secretObj).length > 0 ||
      Object.values(secretObj)[0].length > 3
    ) {
      return Object.values(secretObj)[0][2].toString()
    }
  } else {
    bus.disconnect()

    const capitalizedOsCryptName = `${osCryptName
      .charAt(0)
      .toUpperCase()}${osCryptName.slice(1)}`
    const cookieEncryptionKey = keytar.getPassword(
      `${capitalizedOsCryptName} Keys`,
      `${capitalizedOsCryptName} Safe Storage`
    )
    if (typeof cookieEncryptionKey === "string") {
      return cookieEncryptionKey
    }
  }

  return DEFAULT_ENCRYPTION_KEY
}

const expandUser = (text: string): string => {
  return text.replace(/^~([a-z]+|\/)/, (_, $1: string) => {
    return $1 === "/" ? `${homedir()}/` : `${dirname(homedir())}/${$1}`
  })
}

const expandPaths = (paths: string[]): string => {
  // expand the path of file and remove invalid files
  paths = paths.map(expandUser)

  paths = paths.filter(fs.existsSync)

  // Get the first valid path for now
  return paths.length > 0 ? paths[0] : ""
}

const decryptCookies = async (
  encryptedCookies: Cookie[],
  encryptKey: Buffer
): Promise<Cookie[]> => {
  const cookies: Cookie[] = []

  const numOfCookise = encryptedCookies.length
  for (let i = 0; i < numOfCookise; i++) {
    const cookie = await decryptCookie(encryptedCookies[i], encryptKey)
    cookie !== undefined && cookies.push(cookie)
  }

  console.log(
    "cookies are",
    cookies.map((cookie) => cookie.host_key)
  )

  return cookies
}

const decryptCookie = async (
  cookie: Cookie,
  encryptKey: Buffer
): Promise<Cookie | undefined> => {
  try {
    if (typeof cookie.value === "string" && cookie.value.length > 0)
      return cookie

    if (cookie.encrypted_value.toString().length === 0) return cookie

    const encryptionPrefix = cookie.encrypted_value.toString().substring(0, 3)
    if (encryptionPrefix !== "v10" && encryptionPrefix !== "v11") return cookie

    cookie.encrypted_prefix = encryptionPrefix

    const iv = Buffer.from(Array(17).join(" "), "binary")

    const decipher = await crypto.createDecipheriv(
      "aes-128-cbc",
      encryptKey,
      iv
    )

    decipher.setAutoPadding(false)

    let encryptedData: Buffer = Buffer.from("")
    if (
      typeof cookie.encrypted_value !== "number" &&
      typeof cookie.encrypted_value !== "string"
    ) {
      encryptedData = cookie.encrypted_value.slice(3)
    }

    let decoded = decipher.update(encryptedData)

    const final = decipher.final()
    final.copy(decoded, decoded.length - 1)

    const padding = decoded[decoded.length - 1]
    if (padding !== 0) decoded = decoded.slice(0, decoded.length - padding)

    const decodedBuffer = decoded.toString("utf8")

    const originalText = decodedBuffer
    cookie.decrypted_value = originalText

    // We don't want to upload Google cookies because Google will detect that these are not coming
    // from the right browser and prevent users from signing in
    if (cookie.host_key?.toString()?.includes("google")) return undefined

    return cookie
  } catch (err) {
    console.error("Decrypt failed with error: ", err)
    console.error("The cookie that failed was", cookie)
    return undefined
  }
}

const getCookiesFromFile = async (
  browser: InstalledBrowser
): Promise<Cookie[]> => {
  const cookieFile = expandPaths(getCookieFilePath(browser))

  try {
    const tempFile = createLocalCopy(cookieFile)

    const db = knex({
      client: "sqlite3",
      connection: {
        filename: tempFile,
      },
    })

    const rows: Cookie[] = await db.select().from<Cookie>("cookies")

    return rows
  } catch (err) {
    console.error("Could not get cookies from file. Error:", err)
    return []
  }
}

const getBookmarksFromFile = (browser: InstalledBrowser): string => {
  const bookmarkFile = expandPaths(getBookmarkFilePath(browser))

  try {
    const bookmarks = fs.readFileSync(bookmarkFile, "utf8")
    const bookmarksJSON = JSON.parse(bookmarks)

    // Remove checksum if it exists
    delete bookmarksJSON.checksum

    return JSON.stringify(bookmarksJSON)
  } catch (err) {
    console.error("Could not get bookmarks from file. Error:", err)
    return ""
  }
}

const getExtensionIDs = (browser: InstalledBrowser): string => {
  const extensionsDir = expandPaths(getExtensionDir(browser))

  try {
    // Get all the directory names as it is the extension's ID
    const extensions = fs
      .readdirSync(extensionsDir, { withFileTypes: true })
      .filter((dirent) => dirent.isDirectory() && dirent.name !== "Temp")
      .map((dirent) => dirent.name)
    return extensions.toString()
  } catch (err) {
    console.error("Could not get extensions IDs. Error:", err)
    return ""
  }
}

const getCookieEncryptionKey = async (
  browser: InstalledBrowser
): Promise<Buffer> => {
  const salt = "saltysalt"
  const length = 16
  switch (process.platform) {
    case "darwin": {
      let myPass = await keytar.getPassword(
        getKeyService(browser),
        getKeyUser(browser)
      )

      if (myPass === null) myPass = DEFAULT_ENCRYPTION_KEY

      const iterations = 1003 // number of pbkdf2 iterations on mac
      const key = crypto.pbkdf2Sync(myPass, salt, iterations, length, "sha1")
      return key
    }
    case "linux": {
      const myPass = await getLinuxCookieEncryptionKey(browser)
      const iterations = 1

      const key = await crypto.pbkdf2Sync(
        myPass,
        salt,
        iterations,
        length,
        "sha1"
      )

      return key
    }
    default:
      throw Error("OS not recognized. Works on OSX or linux.")
  }
}

const createLocalCopy = (cookieFile: string): string => {
  if (fs.existsSync(cookieFile)) {
    const tmpFile = tmp.fileSync({ postfix: ".sqlite" })

    const data = fs.readFileSync(cookieFile)
    fs.writeFileSync(tmpFile.name, data)

    return tmpFile.name
  } else {
    throw Error(`Can not find cookie file at: ${cookieFile}`)
  }
}

const isBrowserInstalled = (browser: InstalledBrowser) => {
  return fs.existsSync(expandPaths(getCookieFilePath(browser)))
}

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

const getDecryptedCookies = async (
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

  const bookmarks = getBookmarksFromFile(browser)

  if (bookmarks.length === 0) return undefined

  return JSON.stringify(bookmarks)
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

export {
  InstalledBrowser,
  getInstalledBrowsers,
  getDecryptedCookies,
  getBookmarks,
  getExtensions,
}
