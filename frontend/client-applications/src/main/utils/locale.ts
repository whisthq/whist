import { execCommand } from "@app/main/utils/execCommand"
import { matchServerLanguageWithRegionFormat } from "@app/main/utils/matchServerLanguages"
import { windowsKey } from "@app/main/utils/windowsRegistry"

const getLocaleMacAndLinux = () => {
  // This function will get the user locale on Mac/Linux clients
  const userLocales: { [key: string]: string } = {}
  const userLocaleRaw = execCommand("locale", ".", {}, "pipe")
  const parsedUserLocale = userLocaleRaw?.toString()?.split(/\r?\n/) ?? ""
  for (const s of parsedUserLocale) {
    const keyValuePair = s.split(/=/)
    if (
      keyValuePair.length === 2 &&
      keyValuePair[0].length >= 1 &&
      keyValuePair[1].length >= 1 &&
      !keyValuePair[0].includes("LANG")
    ) {
      userLocales[keyValuePair[0]] = keyValuePair[1].split('"').join("")
    }
  }
  return userLocales
}

const getLocaleWindows = () => {
  // This function will get the user locale on Windows clients
  const userLocales: { [key: string]: string } = {}
  const userLocaleRaw: string =
    windowsKey("Control Panel\\International").getValue("LocaleName") ?? ""

  // Remove all new-lines and split over spaces to select last word (containing the locale)
  const parsedUserLocale = userLocaleRaw
    .toString()
    .replace(/(\r\n|\n|\r)/gm, "")
    .split(" ")
  // Extract locale, transform them into the `language_region` format
  const windowsLocale = matchServerLanguageWithRegionFormat(
    parsedUserLocale[parsedUserLocale.length - 1].split("-").join("_")
  )
  if (windowsLocale !== "") {
    userLocales.LC_ALL = windowsLocale
  } else {
    console.log(
      "Could not find a match for client locale" +
        parsedUserLocale[parsedUserLocale.length - 1].split("-").join("_") +
        ". Using C instead"
    )
    // If the locale is not supported, just use the standard POSIX (C) linux locale.
    userLocales.LC_ALL = "C"
  }
  return userLocales
}

export const getUserLocale = () => {
  const currentPlatform = process.platform
  if (currentPlatform === "darwin" || currentPlatform === "linux") {
    return getLocaleMacAndLinux()
  } else {
    return getLocaleWindows()
  }
}
