import { execCommandByOS } from "@app/main/utils/execCommand"
import {
  linuxLanguages,
  linuxLanguageToDefaultRegion,
} from "@app/constants/linuxLanguages"

const getUserLanguage = () => {
  const userLanguageRaw = execCommandByOS(
    "defaults read -g AppleLocale",
    'locale | grep "LANG"',
    "",
    ".",
    {},
    "pipe"
  )

  const lowerCasedLinuxLanguages = linuxLanguages.map((element) => {
    return element.toLowerCase()
  })
  const caseInsensitiveLanguageSearch = (language: string) => {
    const linuxLanguagesIndex = lowerCasedLinuxLanguages.indexOf(
      language.toLowerCase()
    )
    if (linuxLanguagesIndex !== -1) {
      return linuxLanguages[linuxLanguagesIndex]
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
      // If no region is provided and the script is Traditional Chinese, we need to set the region to Taiwan (or Hong Kong), otherwise the default region will use the Simplified script
      if (languageAndScript[1].includes("Hant")) {
        // Add default region for Traditional Chinese script
        return searchLanguageWithRegion(languageAndScript[0] + "_" + "TW")
      } else {
        // Remove the script and find a best match for the language
        return searchLanguageWithoutRegion(languageAndScript[0])
      }
    } else if (languageAndScript[1].split("_").length === 2) {
      // Handle the [language designator]-[script designator]_[region designator] case
      // If the script is Traditional Chinese, we need to set the overwrite the region and use Taiwan (or Hong Kong), otherwise the Simplified script might be used instead
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
  }

  const currentPlatform = process.platform
  if (currentPlatform === "darwin") {
    // Remove newlines and the encoding if found
    const language =
      userLanguageRaw
        ?.toString()
        ?.replace(/(\r\n|\n|\r)/gm, "")
        ?.split(".")[0] ?? ""
    // Check if a script is present
    if (language.includes("-")) {
      return searchLanguageWithScript(language)
    } else if (language.split("_").length === 2) {
      return searchLanguageWithRegion(language)
    } else {
      return searchLanguageWithoutRegion(language)
    }
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

export { getUserLanguage, getUserLocale }
