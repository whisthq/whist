import {
  linuxSupportedLanguages,
  linuxLanguageToDefaultRegion,
} from "@app/constants/mandelboxLanguages"
import _ from "lodash"

export const caseInsensitiveLanguageSearch = (language: string) => {
  return _.find(
    linuxSupportedLanguages,
    (linuxLang) => language.toLowerCase() === linuxLang.toLowerCase()
  )
}

// Check if a language in the form [language designator] is available on the mandelbox
export const matchServerLanguageFormat = (language: string) => {
  // Check if the language can be found (using a case insensitive search). If not, check
  // if the language can be found after adding the default region name
  return (
    caseInsensitiveLanguageSearch(language) ??
    caseInsensitiveLanguageSearch(
      language.toLowerCase() +
        "_" +
        linuxLanguageToDefaultRegion[language.toLowerCase()]
    ) ??
    ""
  )
}

// Check if a language in the form [language designator]_[region designator] is available on the mandelbox
export const matchServerLanguageWithRegionFormat = (language: string) => {
  // Check if the language can be found (using a case insensitive search). If not found, check if the language
  // can be found after removing the region or after replacing the region with the default one
  return (
    caseInsensitiveLanguageSearch(language) ??
    matchServerLanguageFormat(language.split("_")[0])
  )
}

export const matchServerLanguageWithScriptFormat = (language: string) => {
  const languageAndScript = language.split("-")
  if (languageAndScript[1].split("_").length === 1) {
    // Handle the [language designator]-[script designator] case. No region has been provided
    // If no region is provided and the script is Traditional Chinese, we need to set the region
    // to Taiwan (or Hong Kong), otherwise the default region will use the Simplified script
    if (languageAndScript[1].includes("Hant")) {
      // Add default region for Traditional Chinese script
      return matchServerLanguageWithRegionFormat(
        languageAndScript[0] + "_" + "TW"
      )
    } else {
      // Remove the script and find a best match for the language
      return matchServerLanguageFormat(languageAndScript[0])
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
      return matchServerLanguageWithRegionFormat(
        languageAndScript[0] + "_" + "TW"
      )
    } else if (
      languageAndScript[1].includes("Hans") &&
      !languageAndScript[1].split("_")[1].includes("CN") &&
      !languageAndScript[1].split("_")[1].includes("SG")
    ) {
      // Search using "TW" as the region, instead of the one detected
      return matchServerLanguageWithRegionFormat(
        languageAndScript[0] + "_" + "CN"
      )
    } else {
      // Remove the script and find a best match for the language
      const regionName = languageAndScript[1].split("_")[1]
      return matchServerLanguageWithRegionFormat(
        languageAndScript[0] + "_" + regionName
      )
    }
  }
  return ""
}
