import { execCommand } from "@app/main/utils/execCommand"
import { searchLanguageWithRegion } from "@app/main/utils/serverLanguages"

const currentPlatform = process.platform

export const getUserLocale = () => {
  const userLocales: { [key: string]: string } = {}
  if (currentPlatform === "darwin" || currentPlatform === "linux") {
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
  } else {
    // This function will return the locale in the Windows client format
    const userLocaleRaw = execCommand(
      'reg query "HKCU\\Control Panel\\International" /v LocaleName | findstr /c:"LocaleName"',
      ".",
      {},
      "pipe"
    )
    // Remove all new-lines and split over spaces to select last word (containing the locale)
    const parsedUserLocale = userLocaleRaw
      .toString()
      .replace(/(\r\n|\n|\r)/gm, "")
      .split(" ")
    // Extract locale, transform them into the `language_region` format
    const windowsLocale = searchLanguageWithRegion(
      parsedUserLocale[parsedUserLocale.length - 1].split("-").join("_")
    )
    if (windowsLocale !== "") {
      userLocales.LC_ALL = windowsLocale
    } else {
      // If the locale is not supported, just use the standard C linux locale.
      console.log(
        "Could not find a match for client locale" +
          parsedUserLocale[parsedUserLocale.length - 1].split("-").join("_") +
          ". Using C instead"
      )
      userLocales.LC_ALL = "C"
    }
  }
  return userLocales
}
