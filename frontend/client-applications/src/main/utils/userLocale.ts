import { execCommandByOS } from "@app/main/utils/execCommand"

const getUserLanguage = () => {
  const userLanguageRaw = execCommandByOS(
    "defaults read -g AppleLocale",
    'locale | grep "LANG"',
    "",
    ".",
    {},
    "pipe"
  )
  const currentPlatform = process.platform
  if (currentPlatform === "darwin") {
    return userLanguageRaw?.toString() ?? ""
  } else if (currentPlatform === "linux") {
    const parsedUserLanguage = userLanguageRaw?.toString()?.split(/\r?\n/) ?? ""
    for (const expr of ["LANGUAGE", "LANG"]) {
      if (parsedUserLanguage.includes(expr)) {
        for (const s of parsedUserLanguage) {
          const keyValuePair = s.split(/=/)
          if (
            keyValuePair.length === 2 &&
            keyValuePair[1].length >= 1 &&
            keyValuePair[0].includes(expr)
          ) {
            return keyValuePair[1].split('"').join("")
          }
        }
      }
    }
  }
}

const getUserLocale = () => {
  const userLocaleRaw = execCommandByOS("locale", "locale", "", ".", {}, "pipe")
  const parsedUserLocale = userLocaleRaw?.toString()?.split(/\r?\n/) ?? ""
  const userLocaleDictionary: { [key: string]: string } = {}
  for (const s of parsedUserLocale) {
    const keyValuePair = s.split(/=/)
    if (keyValuePair.length === 2 && keyValuePair[1].length >= 1) {
      userLocaleDictionary[keyValuePair[0]] = keyValuePair[1]
        .split('"')
        .join("")
    }
  }
  return userLocaleDictionary
}

export { getUserLanguage, getUserLocale }
