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
} from "@app/utils/constants"
import keytar from "keytar"
import dbus from "dbus-next"
import fs from "fs"
import tmp from "tmp"
import { homedir } from "os"
import { dirname } from "path"
import Database from "better-sqlite3"
import crypto from "crypto"

interface Cookie {
  [key: string]: Buffer | string | number
}

const getCookies = async (browser: string): Promise<Cookie[]> => {
  const encryptedCookies = await getCookiesFromFile(browser)
  // console.log(`Encrypted Cookies ${encryptedCookies}`);

  const encryptKey = await getCookieEncryptionKey(browser)

  const cookies = await decryptCookies(encryptedCookies, encryptKey)

  return cookies
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

  return cookies
}

const decryptCookie = async (
  cookie: Cookie,
  encryptKey: Buffer
): Promise<Cookie | undefined> => {
  try {
    if (typeof cookie.value === "string" && cookie.value.length > 0) {
      return cookie
    }

    const encryptionPrefix = cookie.encrypted_value.toString().substring(0, 3)
    // if (encryptionPrefix !== "v10" && encryptionPrefix !== "v11") {
    //   return cookie
    // }

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
      typeof cookie.encrypted_value != "number" &&
      typeof cookie.encrypted_value != "string"
    ) {
      encryptedData = cookie.encrypted_value.slice(3)
    }

    let decoded = decipher.update(encryptedData)

    const final = decipher.final()
    final.copy(decoded, decoded.length - 1)

    const padding = decoded[decoded.length - 1]
    if (padding) {
      decoded = decoded.slice(0, decoded.length - padding)
    }

    const decodedBuffer = decoded.toString("utf8")

    const originalText = decodedBuffer
    cookie.decrypted_value = originalText

    return cookie
  } catch (err) {
    console.error("Decrypt failed with error: ", err)
    console.error("The cookie that failed was", cookie)
    return undefined
  }
}

const getCookiesFromFile = async (browser: string): Promise<Cookie[]> => {
  const cookieFile = getExpandedCookieFilePath(browser)

  const tempFile = createLocalCopy(cookieFile)

  const db = new Database(tempFile, {
    fileMustExist: true,
    verbose: console.log,
  })

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
  // console.log(`The platform we are on is ${process.platform}`);

  switch (process.platform) {
    case "darwin": {
      return expandPaths(getCookieFilePath(browser), "osx")
    }
    case "linux": {
      return expandPaths(getCookieFilePath(browser), "linux")
    }
    default:
      throw Error("OS not recognized. Works on OSX or linux.")
  }
}

const getCookieEncryptionKey = async (browser: string): Promise<Buffer> => {
  const salt = "saltysalt"
  const length = 16
  switch (process.platform) {
    case "darwin": {
      let myPass = await keytar.getPassword(
        getKeyService(browser),
        getKeyUser(browser)
      )

      if (myPass === null) {
        myPass = "peanuts"
      }

      console.log("my pass is", myPass)

      const iterations = 1003 // number of pbkdf2 iterations on mac
      const key = crypto.pbkdf2Sync(myPass, salt, iterations, length, "sha1")
      return key
    }
    case "linux": {
      const myPass = await getLinuxCookieEncryptionKey(getOsCryptName(browser))
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
    console.log(`New tempfile named ${tmpFile.name}`)

    const data = fs.readFileSync(cookieFile)
    fs.writeFileSync(tmpFile.name, data)

    return tmpFile.name
  } else {
    throw Error(`Can not find cookie file at: ${cookieFile}`)
  }
}

const expanduser = (text: string): string => {
  console.log(
    `The homedir is ${homedir()}, text is ${text}, dir home is ${dirname(
      homedir()
    )},`
  )

  return text.replace(/^~([a-z]+|\/)/, (_, $1: string) => {
    console.log(`1 is ${$1} and _ is ${_}`)

    return $1 === "/" ? `${homedir()}/` : `${dirname(homedir())}/${$1}`
  })
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

const getKeyService = (browser: string): string => {
  return getKeyUser(browser) + " Safe Storage"
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
      return "brave"
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

const test = async (): Promise<void> => {
  await getCookies("chrome")
}

test()
