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

const bus = dbus.sessionBus()

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
  osCryptName: string
): Promise<string> => {
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

    const secretObj: { string: Array<string | any> } =
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
