import { execCommand } from "@app/main/utils/execCommand"
import {
  linuxSupportedLanguages,
  linuxLanguageToDefaultRegion,
  browserSupportedLanguages,
} from "@app/constants/mandelboxLanguages"

const getUserLanguages = () => {
  const lowerCasedlinuxSupportedLanguages = linuxSupportedLanguages.map(
    (element) => {
      return element.toLowerCase()
    }
  )
  const caseInsensitiveLanguageSearch = (language: string) => {
    const linuxSupportedLanguagesIndex =
      lowerCasedlinuxSupportedLanguages.indexOf(language.toLowerCase())
    if (linuxSupportedLanguagesIndex !== -1) {
      return linuxSupportedLanguages[linuxSupportedLanguagesIndex]
    }
    return ""
  }

  // Check if a language in the form [language designator] is available on the docker container
  const searchLanguageWithoutRegion = (language: string) => {
    // Check if the language can be found (using a case insensitive search)
    const ubuntuLanguage = caseInsensitiveLanguageSearch(language)
    if (ubuntuLanguage !== "") {
      return ubuntuLanguage
    }
    // Check if the language can be found after adding the default region name
    if (language.toLowerCase() in linuxLanguageToDefaultRegion) {
      const ubuntuDefaultRegionLanguage = caseInsensitiveLanguageSearch(
        language.toLowerCase() +
          "_" +
          linuxLanguageToDefaultRegion[language.toLowerCase()]
      )
      if (ubuntuDefaultRegionLanguage !== "") {
        return ubuntuDefaultRegionLanguage
      }
    }
    return ""
  }

  // Check if a language in the form [language designator]_[region designator] is available on the Docker container
  const searchLanguageWithRegion = (language: string) => {
    // Check if the language can be found (using a case insensitive search)
    const ubuntuLanguage: string = caseInsensitiveLanguageSearch(language)
    if (ubuntuLanguage !== "") {
      return ubuntuLanguage
    }
    // Check if the language can be found after removing the region or after replacing the region with the default one
    return searchLanguageWithoutRegion(language.split("_")[0])
  }

  const searchLanguageWithScript = (language: string) => {
    const languageAndScript = language.split("-")
    if (languageAndScript[1].split("_").length === 1) {
      // Handle the [language designator]-[script designator] case. No region has been provided
      // If no region is provided and the script is Traditional Chinese, we need to set the region
      // to Taiwan (or Hong Kong), otherwise the default region will use the Simplified script
      if (languageAndScript[1].includes("Hant")) {
        // Add default region for Traditional Chinese script
        return searchLanguageWithRegion(languageAndScript[0] + "_" + "TW")
      } else {
        // Remove the script and find a best match for the language
        return searchLanguageWithoutRegion(languageAndScript[0])
      }
    } else if (languageAndScript[1].split("_").length === 2) {
      // Handle the [language designator]-[script designator]_[region designator] case
      // If the script is Traditional Chinese, we need to set the overwrite the region and use
      // Taiwan (or Hong Kong), otherwise the Simplified script might be used instead
      if (
        languageAndScript[1].includes("Hant") &&
        !languageAndScript[1].split("_")[1].includes("HK") &&
        !languageAndScript[1].split("_")[1].includes("TW")
      ) {
        // Search using "TW" as the region, instead of the one detected
        return searchLanguageWithRegion(languageAndScript[0] + "_" + "TW")
      } else if (
        languageAndScript[1].includes("Hans") &&
        !languageAndScript[1].split("_")[1].includes("CN") &&
        !languageAndScript[1].split("_")[1].includes("SG")
      ) {
        // Search using "TW" as the region, instead of the one detected
        return searchLanguageWithRegion(languageAndScript[0] + "_" + "CN")
      } else {
        // Remove the script and find a best match for the language
        const regionName = languageAndScript[1].split("_")[1]
        return searchLanguageWithRegion(languageAndScript[0] + "_" + regionName)
      }
    }
    return ""
  }
  const currentPlatform = process.platform

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
        'reg query "HCKU\Control Panel\International\User Profile" /v Languages | findstr /c:"Languages"',
        ".",
        {},
        "pipe"
      )
      const parsedLanguagesRaw = languagesRaw.toString().replace(/(\r\n|\n|\r)/gm, "").split(" ")
      return parsedLanguagesRaw[parsedLanguagesRaw.length - 1].split("\0").join(":").split("-").join("_")
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
      return ""
    })
    .filter((e) => e)

  // Add fallback options and remove duplicates
  const uniquebrowserLanguages = [...browserLanguages, "en-US", "en"].filter(
    (c, index) => {
      return [...browserLanguages, "en-US", "en"].indexOf(c) === index
    }
  )

  return {
    systemLanguages: systemLanguages.join(":"),
    browserLanguages: uniquebrowserLanguages.join(","),
  }
}

const getUserLocale = () => {
  const userLocaleRaw = execCommand("locale", ".", {}, "pipe")
  const parsedUserLocale = userLocaleRaw?.toString()?.split(/\r?\n/) ?? ""
  const userLocaleDictionary: { [key: string]: string } = {}
  for (const s of parsedUserLocale) {
    const keyValuePair = s.split(/=/)
    if (
      keyValuePair.length === 2 &&
      keyValuePair[0].length >= 1 &&
      keyValuePair[1].length >= 1 &&
      !keyValuePair[0].includes("LANG")
    ) {
      userLocaleDictionary[keyValuePair[0]] = keyValuePair[1]
        .split('"')
        .join("")
    }
  }
  return userLocaleDictionary
}

export { getUserLanguages, getUserLocale }
