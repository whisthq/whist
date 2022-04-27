import { browserSupportedLanguages } from "@app/constants/languages"
import { execCommand } from "@app/main/utils/execCommand"
import {
  matchServerLanguageFormat,
  matchServerLanguageWithRegionFormat,
  matchServerLanguageWithScriptFormat,
} from "@app/main/utils/matchServerLanguages"
import uniq from "lodash.uniq"
import { windowsKey } from "@app/main/utils/windowsRegistry"
const currentPlatform = process.platform

const getMacLanguageRaw = () => {
  // This function will return the language in the Mac client format
  const languagesRaw = execCommand(
    "defaults read -g AppleLocale",
    ".",
    {},
    "pipe"
  )
  // Remove newlines
  return languagesRaw.toString().replace(/(\r\n|\n|\r)/gm, "")
}

const getLinuxLanguageRaw = () => {
  // This function will return the language (or list of languages separated by ':') in the Linux client format
  const languagesRaw = execCommand('locale | grep "LANG"', ".", {}, "pipe")
  // Split at newline
  const parsedLanguages = languagesRaw.toString().split(/\r?\n/)
  for (const expr of ["LANGUAGE", "LANG"]) {
    for (const s of parsedLanguages) {
      const keyValuePair = s.split(/=/)
      if (
        keyValuePair.length === 2 &&
        keyValuePair[1].length >= 1 &&
        keyValuePair[0].includes(expr)
      ) {
        // Peel off double quotes
        return keyValuePair[1].split('"').join("")
      }
    }
  }
  return ""
}

const getWindowsLanguageRaw = () => {
  // This function will return the language in the Windows client format
  const languagesRaw: string =
    windowsKey("Control Panel\\International\\User Profile").getValue(
      "Languages"
    ) ?? ""
  // Remove all new-lines and split over spaces to select last word (containing the languages)
  const parsedLanguagesRaw = languagesRaw
    .toString()
    .replace(/(\r\n|\n|\r)/gm, "")
    .split(" ")
  // Extract languages, separate them with colons, and transform them into the `language_region` format
  return parsedLanguagesRaw[parsedLanguagesRaw.length - 1]
    .split("\\0")
    .join(":")
    .split("-")
    .join("_")
}

const parseUserLanguagesRaw = () => {
  if (currentPlatform === "darwin") {
    return getMacLanguageRaw()
  } else if (currentPlatform === "linux") {
    return getLinuxLanguageRaw()
  } else {
    return getWindowsLanguageRaw()
  }
}

const macRawLanguageToServerFormat = (language: string) => {
  if (language.includes("-")) {
    // Check if the language is in the [language designator]-[script designator] format
    // or the [language designator]-[script designator]_[region designator] format
    return matchServerLanguageWithScriptFormat(language)
  } else if (language.split("_").length === 2) {
    return matchServerLanguageWithRegionFormat(language)
  } else {
    return matchServerLanguageFormat(language)
  }
}

const linuxRawLanguageToServerFormat = (language: string) => {
  // If the client is running on Linux, the language is already in the format required by the server
  return language
}

const windowsRawLanguageToServerFormat = (language: string) => {
  if (language.split("_").length === 2) {
    return matchServerLanguageWithRegionFormat(language)
  } else {
    return matchServerLanguageFormat(language)
  }
}

export const getUserLanguages = () => {
  const systemLanguages = parseUserLanguagesRaw()
    .split(":")
    .map((currentElement: string) => {
      const language = currentElement.split(".")[0] // Remove encoding
      if (currentPlatform === "darwin") {
        return macRawLanguageToServerFormat(language)
      } else if (currentPlatform === "linux") {
        return linuxRawLanguageToServerFormat(language)
      } else {
        return windowsRawLanguageToServerFormat(language)
      }
    })

  const browserLanguages = systemLanguages
    .map((language: string) => {
      if (browserSupportedLanguages.includes(language)) {
        return language
      }
      const noUnderscoresLanguage = language.split("_").join("-")
      if (browserSupportedLanguages.includes(noUnderscoresLanguage)) {
        return noUnderscoresLanguage
      }
      const noRegionLanguage = language.split("_")[0]
      if (browserSupportedLanguages.includes(noRegionLanguage)) {
        return noRegionLanguage
      }
      // If the language is not supported, just use English
      console.log(
        "Could not find a match for client language" +
          language +
          ". Using English instead"
      )
      return ""
    })
    .filter((e) => e)

  return {
    systemLanguages: systemLanguages.join(":"),
    // Add fallback options (english), remove duplicates, and join values in a single string
    browserLanguages: uniq([...browserLanguages, "en-US", "en"]).join(","),
  }
}
