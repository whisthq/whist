import { browserSupportedLanguages } from "@app/constants/mandelboxLanguages"
import { execCommand } from "@app/main/utils/execCommand"
import {
  searchLanguageWithoutRegion,
  searchLanguageWithRegion,
  searchLanguageWithScript,
} from "@app/main/utils/serverLanguages"
import _ from "lodash"
const currentPlatform = process.platform

export const getUserLanguages = () => {
  const parseUserLanguages = () => {
    if (currentPlatform === "darwin") {
      // This function will return the language in the Mac client format
      const languagesRaw = execCommand(
        "defaults read -g AppleLocale",
        ".",
        {},
        "pipe"
      )
      // Remove newlines
      return languagesRaw.toString().replace(/(\r\n|\n|\r)/gm, "")
    } else if (currentPlatform === "linux") {
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
    } else {
      // This function will return the language in the Windows client format
      const languagesRaw = execCommand(
        'reg query "HKCU\\Control Panel\\International\\User Profile" /v Languages | findstr /c:"Languages"',
        ".",
        {},
        "pipe"
      )
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
  }

  const systemLanguages = parseUserLanguages()
    .split(":")
    .map((currentElement: string) => {
      // Remove encoding
      const language = currentElement.split(".")[0]
      if (currentPlatform === "darwin") {
        // Check if a script is present
        if (language.includes("-")) {
          return searchLanguageWithScript(language)
        } else if (language.split("_").length === 2) {
          return searchLanguageWithRegion(language)
        } else {
          return searchLanguageWithoutRegion(language)
        }
      } else if (currentPlatform === "linux") {
        return language
      } else {
        if (language.split("_").length === 2) {
          return searchLanguageWithRegion(language)
        } else {
          return searchLanguageWithoutRegion(language)
        }
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
    browserLanguages: _.uniq([...browserLanguages, "en-US", "en"]).join(","),
  }
}
