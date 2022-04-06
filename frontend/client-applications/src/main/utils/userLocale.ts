import { execCommandByOS } from "@app/main/utils/execCommand"

const getUserLocale = () => {
  const userLocaleRaw = execCommandByOS(
    "locale",
    "locale",
    "",
    ".",
    {},
    "pipe"
  )
  console.log("LOCALE")
  console.log(userLocaleRaw)

  const parsedUserLocale = userLocaleRaw?.toString()?.split(/\r?\n/) ?? ""
  const userLocaleDictionary = {}
  for (const s of parsedUserLocale) {
    const keyValuePair = s.split(/=/)
    if (keyValuePair.length === 2) {
      userLocaleDictionary[keyValuePair[0]] = keyValuePair[1]
        .split('"')
        .join("")
    }
  }
  return userLocaleDictionary
}

export { getUserLocale }
