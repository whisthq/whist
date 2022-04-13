import { execCommandByOS } from "@app/main/utils/execCommand"

const getUserLocale = () => {
  const userLocaleRaw = execCommandByOS(
    //"defaults read -g AppleLocale",
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
  const userLocaleDictionary: { [key: string]: string } = {}
  for (const s of parsedUserLocale) {
    const keyValuePair = s.split(/=/)
    if (keyValuePair.length === 2 && keyValuePair[1].length >= 1) {
      userLocaleDictionary[keyValuePair[0]] = keyValuePair[1]
        .split('"')
        .join("")
    }
  }
  userLocaleDictionary["LANGUAGE"] = "it_IT"
  return userLocaleDictionary
}

export { getUserLocale }
