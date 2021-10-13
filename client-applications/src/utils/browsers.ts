/**
 * Copyright Fractal Computers, Inc. 2021
 * @file browsers.ts
 * @brief This file contains support to get cookies from browser.
 */
import {
  ALGORITHM_PLAIN,
  BraveLinuxCookieFiles,
  BraveOSXCookieFiles,
  BusSecretName,
  BusSecretPath,
  ChromeLinuxCookieFiles,
  ChromeOSXCookieFiles,
  ChromiumLinuxCookieFiles,
  ChromiumOSXCookieFiles,
  EdgeLinuxCookieFiles,
  EdgeOSXCookieFiles,
  OperaLinuxCookieFiles,
  OperaOSXCookieFiles,
  SecretServiceName,
} from "./constants"
import * as keytar from "keytar"
import * as dbus from "dbus-next"
import * as fs from "fs"
import tempfile from "tempfile"
import { homedir } from "os"
import { dirname } from "path"
import * as pbkdf2 from "pbkdf2"
import Database from "better-sqlite3"
import { AES, enc } from "crypto-js"

interface Cookie {
  [key: string]: Buffer | string | number
}

const bus = dbus.sessionBus()


const test = async (): Promise<void> => {
  console.log(await getCookies("chrome"))
  
}

test()

const getCookies = async (browser: string): Promise<Cookie[]> => {
  const encryptedCookies = await getCookiesFromFile(browser)
  const encryptKey = await getCookieEncryptionKey(browser)

  const cookies = decryptCookies(encryptedCookies, encryptKey)

  return await cookies
}

const decryptCookies = async (
  encryptedCookies: Cookie[],
  encryptKey: string
): Promise<Cookie[]> => {
  const cookies: Cookie[] = []

  const numOfCookise = encryptedCookies.length
  for (let i = 0; i < numOfCookise; i++) {
    cookies.push(await decryptCookie(encryptedCookies[i], encryptKey))
  }

  return cookies
}

const decryptCookie = async (
  cookie: Cookie,
  encryptKey: string
): Promise<Cookie> => {
  if (typeof cookie.value === "string" && cookie.value.length > 0) {
    return cookie
  }

  const encryptionPrefix = cookie.encrypted_value.toString().substring(0, 3)
  if (encryptionPrefix !== "v10" && encryptionPrefix !== "v11") {
    return cookie
  }

  cookie.encrypted_prefix = encryptionPrefix

  const encryptedValue = cookie.encrypted_value.toString().substring(3)
  const bytes = AES.decrypt(encryptedValue, encryptKey)
  const originalText = bytes.toString(enc.Utf8)

  cookie.encrypted_value = originalText

  return cookie
}

const getCookiesFromFile = async (browser: string): Promise<Cookie[]> => {
  const cookieFile = getExpandedCookieFilePath(browser)
  const tempFile = createLocalCopy(cookieFile)
  const db = new Database(tempFile, { verbose: console.log })

  let rows: Cookie[]
  try {
    const query = await db.prepare(
      "SELECT creation_utc, top_frame_site_key, host_key, name, value, encrypted_value, path, expires_utc, secure, is_httponly, last_access_utc, has_expires, is_persistent, priority, samesite, source_scheme, source_port, is_same_party FROM cookies;"
    )
    rows = query.all()
  } catch {
    const query = await db.prepare(
      "SELECT creation_utc, top_frame_site_key, host_key, name, value, encrypted_value, path, expires_utc, is_secure, is_httponly, last_access_utc, has_expires, is_persistent, priority, samesite, source_scheme, source_port, is_same_party FROM cookies;"
    )
    rows = query.all()
  }

  return rows
}

const getExpandedCookieFilePath = (browser: string): string => {
  let os = ""
  switch (process.platform) {
    case "darwin": {
      os = "osx"
      break
    }
    case "linux": {
      os = "linux"
      break
    }
    default:
      throw Error("OS not recognized. Works on OSX or linux.")
  }

  return expandPaths(getCookieFilePath(browser), os)
}

const getCookieEncryptionKey = async (browser: string): Promise<string> => {
  const salt: Buffer = Buffer.from("saltysalt", "base64")
  const length = 16
  switch (process.platform) {
    case "darwin": {
      let myPass = await keytar.getPassword(
        getKeyUser(browser),
        getOsCryptName(browser)
      )

      if (myPass === null) {
        myPass = "peanuts"
      }

      const iterations = 1003 // number of pbkdf2 iterations on mac
      const key = await pbkdf2.pbkdf2Sync(myPass, salt, iterations, length)
      return key.toString()
    }
    case "linux": {
      const myPass = await getLinuxCookieEncryptionKey(getOsCryptName(browser))

      const iterations = 1
      const key = pbkdf2.pbkdf2Sync(myPass, salt, iterations, length)
      return key.toString()
    }
    default:
      throw Error("OS not recognized. Works on OSX or linux.")
  }
}

const createLocalCopy = (cookieFile: string): string => {
  if (fs.existsSync(cookieFile)) {
    const tmpFile = tempfile(".sqlite")
    fs.readFile(cookieFile, (err: Error | null, data: Buffer): void => {
      if (err != null) throw err
      fs.writeFile(tmpFile, data, (err) => {
        if (err != null) throw err
      })
    })
    return tmpFile
  } else {
    throw Error(`Can not find cookie file at: ${cookieFile}`)
  }
}

const expanduser = (text: string): string => {
  return text.replace(/^~([a-z]+|\/)/, (_, $1: string) =>
    $1 === "/" ? homedir() : `${dirname(homedir())}/${$1}`
  )
}

const expandPaths = (paths: string[], osName: string): string => {
  osName = osName.toLowerCase()

  // expand the path of file and remove invalid files
  paths = paths.map(expanduser)
  paths = paths.filter(fs.existsSync)

  // Get the first valid path for now
  return paths.length > 0 ? paths[0] : ""
}

const getLinuxCookieEncryptionKey = async (
  browser: string
): Promise<string> => {
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

  return "peanuts"
}

const getKeyUser = (browser: string): string => {
  switch (browser.toLowerCase()) {
    case "chrome": {
      return "Chrome"
    }
    case "opera": {
      return "Opera"
    }
    case "edge": {
      return "Microsoft Edge"
    }
    case "chromium": {
      return "Chromium"
    }
    case "brave": {
      return "Brave"
    }
  }
  return ""
}

const getOsCryptName = (browser: string): string => {
  switch (browser.toLowerCase()) {
    case "chrome": {
      return "chrome"
    }
    case "opera": {
      return "chromium"
    }
    case "edge": {
      return "chromium"
    }
    case "chromium": {
      return "chromium"
    }
    case "brave": {
      return "chromium"
    }
  }
  return ""
}

const getCookieFilePath = (browser: string): string[] => {
  switch (process.platform) {
    case "darwin": {
      switch (browser.toLowerCase()) {
        case "chrome": {
          return ChromeOSXCookieFiles
        }
        case "opera": {
          return OperaOSXCookieFiles
        }
        case "edge": {
          return EdgeOSXCookieFiles
        }
        case "chromium": {
          return ChromiumOSXCookieFiles
        }
        case "brave": {
          return BraveOSXCookieFiles
        }
      }
      break
    }
    case "linux": {
      switch (browser.toLowerCase()) {
        case "chrome": {
          return ChromeLinuxCookieFiles
        }
        case "opera": {
          return OperaLinuxCookieFiles
        }
        case "edge": {
          return EdgeLinuxCookieFiles
        }
        case "chromium": {
          return ChromiumLinuxCookieFiles
        }
        case "brave": {
          return BraveLinuxCookieFiles
        }
      }
      break
    }
  }

  return []
}
