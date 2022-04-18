import keytar from "keytar"
import dbus from "dbus-next"
import fs from "fs"
import tmp from "tmp"
import { homedir } from "os"
import path, { dirname } from "path"
import knex from "knex"
import crypto from "crypto"

import {
  ALGORITHM_PLAIN,
  BusSecretName,
  BusSecretPath,
  SecretServiceName,
  InstalledBrowser,
  ChromeWindowsKeys,
  OperaWindowsKeys,
  EdgeWindowsKeys,
  ChromiumWindowsKeys,
  BraveWindowsKeys,
  DEFAULT_ENCRYPTION_KEY,
  BraveLinuxDefaultDir,
  BraveOSXDefaultDir,
  BraveWindowsDefaultDir,
  ChromeLinuxDefaultDir,
  ChromeOSXDefaultDir,
  ChromeWindowsDefaultDir,
  ChromiumLinuxDefaultDir,
  ChromiumOSXDefaultDir,
  EdgeLinuxDefaultDir,
  EdgeOSXDefaultDir,
  OperaLinuxDefaultDir,
  OperaOSXDefaultDir,
  OperaWindowsDefaultDir,
} from "@app/constants/importer"

const ref = process.platform === "win32" ? require("ref-napi") : undefined
const ffi = process.platform === "win32" ? require("ffi-napi") : undefined
const StructType =
  process.platform === "win32" ? require("ref-struct-di") : undefined

interface Cookie {
  [key: string]: Buffer | string | number
}

interface LocalStorageMap {
  [key: string]: string
}

const cryptUnprotectData = (buffer: Buffer) => {
  /*
    NodeJS implementation of the Win32API CryptUnprotectData function,
    used to decode the Chrome/Brave/etc. cookie encryption key

    Args:
        buffer (Buffer): The encrypted key as a Buffer

    Returns:
        Buffer: The decryped key as a Buffer
    */
  const Struct = StructType(ref)
  const DATA_BLOB = Struct({
    length: ref.types.uint32,
    buf: ref.refType(ref.types.byte),
  })
  const PDATA_BLOB = ref.refType(DATA_BLOB)
  const Crypto = new ffi.Library("Crypt32", {
    CryptUnprotectData: [
      "bool",
      [PDATA_BLOB, "string", "string", "void *", "string", "int", PDATA_BLOB],
    ],
  })

  const dataBlobInput = new DATA_BLOB()
  const dataBlobOutput = new DATA_BLOB()

  dataBlobInput.buf = buffer as any
  dataBlobInput.length = buffer.length

  Crypto.CryptUnprotectData(
    dataBlobInput.ref(),
    null,
    null,
    null as any,
    null,
    0,
    dataBlobOutput.ref()
  )

  return ref.reinterpret(dataBlobOutput.buf, dataBlobOutput.length, 0)
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

const expandUser = (text: string): string => {
  if (process.platform === "win32") {
    return text
      .replace("%LOCALAPPDATA%", process.env.LOCALAPPDATA ?? "")
      .replace("%APPDATA", process.env.APPDATA ?? "")
  } else {
    return text.replace(/^~([a-z]+|\/)/, (_, $1: string) => {
      return $1 === "/" ? `${homedir()}/` : `${dirname(homedir())}/${$1}`
    })
  }
}

const expandPaths = (paths: any[]): string => {
  // Expand the path of file and remove invalid files
  paths = paths.map(expandUser)
  paths = paths.filter(fs.existsSync)

  // Get the first valid path for now
  return paths.length > 0 ? paths[0] : ""
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

const getWindowsKeys = (browser: InstalledBrowser) => {
  switch (browser) {
    case InstalledBrowser.CHROME: {
      return ChromeWindowsKeys
    }
    case InstalledBrowser.OPERA: {
      return OperaWindowsKeys
    }
    case InstalledBrowser.EDGE: {
      return EdgeWindowsKeys
    }
    case InstalledBrowser.CHROMIUM: {
      return ChromiumWindowsKeys
    }
    case InstalledBrowser.BRAVE: {
      return BraveWindowsKeys
    }
    default: {
      return []
    }
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
    case "win32": {
      const keyFile = expandPaths(getWindowsKeys(browser))
      const data = JSON.parse(fs.readFileSync(keyFile).toString())
      const key64 = data.os_crypt.encrypted_key.toString("utf-8")
      const buffer = Buffer.from(key64, "base64").slice(5)

      return cryptUnprotectData(buffer)
    }
    default:
      throw Error("OS not recognized. Works on OSX or linux.")
  }
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
    case "win32": {
      switch (browser) {
        case InstalledBrowser.CHROME: {
          return ChromeWindowsDefaultDir
        }
        case InstalledBrowser.BRAVE: {
          return BraveWindowsDefaultDir
        }
        case InstalledBrowser.OPERA: {
          return OperaWindowsDefaultDir
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

  return browserDirectories.map((dir) =>
    path.join(
      dir,
      process.platform === "win32" ? "Network\\Cookies" : "Cookies"
    )
  )
}

const getBookmarkFilePath = (browser: InstalledBrowser): string[] => {
  const browserDirectories = getBrowserDefaultDirectory(browser)
  return browserDirectories.map((dir) => path.join(dir, "Bookmarks"))
}

const getExtensionFilePath = (browser: InstalledBrowser): string[] => {
  const browserDirectories = getBrowserDefaultDirectory(browser)
  return browserDirectories.map((dir) => path.join(dir, "Extensions"))
}

const getPreferencesFilePath = (browser: InstalledBrowser): string[] => {
  const browserDirectories = getBrowserDefaultDirectory(browser)
  return browserDirectories.map((dir) => path.join(dir, "Preferences"))
}

const getLocalStorageDir = (browser: InstalledBrowser): string[] => {
  const browserDirectories = getBrowserDefaultDirectory(browser)
  return browserDirectories.map((dir) =>
    path.join(dir, "Local Storage", "leveldb")
  )
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

const decryptCookie = async (
  cookie: Cookie,
  encryptKey: Buffer
): Promise<Cookie | undefined> => {
  try {
    if (typeof cookie.value === "string" && cookie.value.length > 0)
      return cookie

    if (cookie.encrypted_value.toString().length === 0) return cookie

    if (typeof cookie.encrypted_value === "string") {
      const _encrypted = Buffer.from(cookie.encrypted_value)
      cookie.encrypted_value = _encrypted
    }

    const encryptionPrefix = cookie.encrypted_value.toString().substring(0, 3)

    if (encryptionPrefix !== "v10" && encryptionPrefix !== "v11") return cookie

    cookie.encrypted_prefix = encryptionPrefix

    const iv =
      process.platform === "win32"
        ? (cookie.encrypted_value as Buffer).slice(3, 15)
        : Buffer.from(Array(17).join(" "), "binary")

    const decipher = (await crypto.createDecipheriv(
      process.platform === "win32" ? "aes-256-gcm" : "aes-128-cbc",
      encryptKey,
      iv
    )) as any

    if (process.platform === "win32") {
      const authTag = (cookie.encrypted_value as Buffer).slice(
        (cookie.encrypted_value as Buffer).length - 16
      )

      decipher.setAuthTag(authTag)
    }

    decipher.setAutoPadding(false)

    let encryptedData: Buffer = Buffer.from("")

    if (
      typeof cookie.encrypted_value !== "number" &&
      typeof cookie.encrypted_value !== "string"
    ) {
      encryptedData =
        process.platform === "win32"
          ? cookie.encrypted_value.slice(15, cookie.encrypted_value.length - 16)
          : cookie.encrypted_value.slice(3)
    }

    let decoded = decipher.update(encryptedData)

    if (decoded.length === 0) return undefined

    const final = decipher.final()

    if (process.platform !== "win32") {
      final.copy(decoded, decoded.length - 1)

      const padding = decoded[decoded.length - 1]
      if (padding !== 0) decoded = decoded.slice(0, decoded.length - padding)
    }

    cookie.decrypted_value = decoded.toString("utf8")

    // We don't want to upload Google cookies because Google will detect that these are not coming
    // from the right browser and prevent users from signing in
    if (cookie.host_key?.toString()?.includes("google")) return undefined

    return cookie
  } catch (err) {
    return undefined
  }
}

const decryptCookies = async (
  encryptedCookies: Cookie[],
  encryptKey: Buffer
): Promise<Cookie[]> => {
  const cookies: Cookie[] = []

  const numOfCookies = encryptedCookies.length
  for (let i = 0; i < numOfCookies; i++) {
    const cookie = await decryptCookie(encryptedCookies[i], encryptKey)
    cookie !== undefined && cookies.push(cookie)
  }

  return cookies
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

const getLocalStorageFromFile = (browser: InstalledBrowser): string => {
  const localStorageDir = expandPaths(getLocalStorageDir(browser))

  try {
    // We are only interested in .ldb, .log, CURRENT, and MANIFEST files
    const relevantFiles = fs
      .readdirSync(localStorageDir, { withFileTypes: true })
      .filter(
        (dirent) =>
          dirent.isFile() &&
          (path.extname(dirent.name) === ".ldb" ||
            path.extname(dirent.name) === ".log" ||
            dirent.name === "CURRENT" ||
            dirent.name.startsWith("MANIFEST"))
      )
      .map((dirent) => dirent.name)

    const data: LocalStorageMap = {}
    for (const file of relevantFiles) {
      const filePath = path.join(localStorageDir, file)
      const fileData = fs.readFileSync(filePath)

      // Base64 encode the binary file data so we can pass as JSON
      data[file] = fileData.toString("base64")
    }

    return JSON.stringify(data)
  } catch (err) {
    console.error("Could not get local storage from files. Error:", err)
    return ""
  }
}

const getExtensionIDs = (browser: InstalledBrowser): string => {
  const extensionsDir = expandPaths(getExtensionFilePath(browser))

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

const getPreferencesFromFile = (browser: InstalledBrowser): string => {
  const preferencesFile = expandPaths(getPreferencesFilePath(browser))

  try {
    const preferences = fs.readFileSync(preferencesFile, "utf8")
    return preferences
  } catch (err) {
    console.error("Could not get preferences from file. Error:", err)
    return ""
  }
}

export {
  getCookieEncryptionKey,
  expandPaths,
  getPreferencesFilePath,
  isBrowserInstalled,
  getCookiesFromFile,
  decryptCookies,
  getBookmarksFromFile,
  getExtensionIDs,
  getLocalStorageFromFile,
  getPreferencesFromFile,
}
