const timeZone = () => Intl.DateTimeFormat().resolvedOptions().timeZone

const darkMode = async () => {
  const theme = new Promise((resolve) => {
    ;(chrome as any).braveTheme.getBraveThemeType((themeType: string) => {
      resolve(themeType === "Dark")
    })
  })

  return await theme
}

const userAgent = () => navigator.userAgent

const platform = async () => {
  const info = new Promise((resolve) => {
    chrome.runtime.getPlatformInfo((platformInfo) => {
      switch (platformInfo.os) {
        case "mac":
          resolve("darwin")
          break
        case "win":
          resolve("win32")
          break
        default:
          resolve("linux")
          break
      }
    })
  })

  return await info
}

// See https://github.com/whisthq/whist/blob/81c6c4430ea661b757a8079d86018bb1a66dca87/frontend/client-applications/src/main/utils/keyRepeat.ts
// for explanations of these magic numbers
const keyboardRepeatRate = async () => {
  const os = await platform()
  switch (os) {
    case "darwin":
      return await new Promise((resolve) =>
        (chrome as any).whist.getKeyboardRepeatRate((rate: number) => {
          resolve(Math.round((1.0 - (rate - 2) / 128) * 29 + 1))
        })
      )
    default:
      return 40 // 40 is the fastest repeat rate on Linux
  }
}

const keyboardRepeatInitialDelay = async () => {
  const os = await platform()

  switch (os) {
    case "darwin":
      return await new Promise((resolve) =>
        (chrome as any).whist.getKeyboardRepeatInitialDelay((delay: number) => {
          resolve(Math.round((1000 / 60) * 1.2 * delay))
        })
      )
    default:
      return 27 // 27 is the lowest delay on Linux
  }
}

// const locale = () => {
//   return chrome.i18n.getUILanguage().replace(/-/g, "_")
// }

const userLocale = async () => {
  return await new Promise((resolve) =>
    (chrome as any).whist.getUserLocale((userLocaleJson: any) => {
      resolve(userLocaleJson)
    })
  )
}

const browserLanguages = async () => {
  const browserLangs = new Promise((resolve) => {
    chrome.i18n.getAcceptLanguages((languages) => {
      resolve(languages.join(",").replace(/-/g, "_"))
    })
  })

  return await browserLangs
}

const systemLanguages = async () => {
  return await new Promise((resolve) =>
    (chrome as any).whist.getSystemLanguage((systemLanguage: string) => {
      resolve(systemLanguage.replace(/-/g, "_"))
    })
  )
}

const location = async () => {
  const IPSTACK_API_KEY = "f3e4e15355710b759775d121e243e39b"

  const response = await fetch(
    `http://api.ipstack.com/check?access_key=${IPSTACK_API_KEY}&format=1`
  )

  const json = (await response.json()) as {
    longitude: number
    latitude: number
  }

  return {
    longitude: json.longitude.toFixed(7),
    latitude: json.latitude.toFixed(7),
  }
}

export {
  timeZone,
  darkMode,
  userAgent,
  platform,
  keyboardRepeatRate,
  keyboardRepeatInitialDelay,
  userLocale,
  browserLanguages,
  systemLanguages,
  location,
}
