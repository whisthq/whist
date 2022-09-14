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

export {
  timeZone,
  darkMode,
  userAgent,
  platform,
  keyboardRepeatRate,
  keyboardRepeatInitialDelay,
}
