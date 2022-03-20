import { execCommandByOS } from "@app/main/utils/execCommand"
import {
  INITIAL_KEY_REPEAT_MAC_TO_LINUX_CONVERSION_FACTOR,
  KEY_REPEAT_RATE_MIN_MAC,
  KEY_REPEAT_RATE_RANGE_LINUX,
  KEY_REPEAT_RATE_RANGE_MAC,
  KEY_REPEAT_RATE_MIN_LINUX,
} from "@app/constants/keyRepeat"

const getInitialKeyRepeat = () => {
  const initialKeyRepeatRaw = execCommandByOS(
    "defaults read NSGlobalDomain InitialKeyRepeat",
    /* eslint-disable no-template-curly-in-string */
    "xset -q | grep 'auto repeat delay'",
    "",
    ".",
    {},
    "pipe"
  )

  let initialKeyRepeat =
    initialKeyRepeatRaw?.toString()?.replace(/\n$/, "") ?? ""

  // Extract value from bash output
  if (process.platform === "linux" && initialKeyRepeat !== "") {
    const startIndex =
      initialKeyRepeat.indexOf("auto repeat delay:") +
      "auto repeat delay:".length +
      2
    const endIndex = initialKeyRepeat.indexOf("repeat rate:") - 4
    initialKeyRepeat = initialKeyRepeat.substring(startIndex, endIndex)
  } else if (process.platform === "darwin" && initialKeyRepeat !== "") {
    // Convert the key repetition delay from Mac scale (shortest=15, longest=120) to Linux scale (shortest=115, longest=INFINITY)
    const initialKeyRepeatFloat =
      parseInt(initialKeyRepeat) *
      INITIAL_KEY_REPEAT_MAC_TO_LINUX_CONVERSION_FACTOR
    initialKeyRepeat = initialKeyRepeatFloat.toFixed()
  }

  const keyRepeatInt = parseInt(initialKeyRepeat)
  return !isNaN(keyRepeatInt) ? keyRepeatInt : undefined
}

const getKeyRepeat = () => {
  const keyRepeatRaw = execCommandByOS(
    "defaults read NSGlobalDomain KeyRepeat",
    /* eslint-disable no-template-curly-in-string */
    "xset -q | grep 'auto repeat delay'",
    "",
    ".",
    {},
    "pipe"
  )

  let keyRepeat = keyRepeatRaw !== null ? keyRepeatRaw.toString() : ""

  // Remove trailing '\n'
  keyRepeat.replace(/\n$/, "")
  // Extract value from bash output
  if (process.platform === "linux" && keyRepeat !== "") {
    const startIndex =
      keyRepeat.indexOf("repeat rate:") + "repeat rate:".length + 2
    const endIndex = keyRepeat.length
    keyRepeat = keyRepeat.substring(startIndex, endIndex)
  } else if (process.platform === "darwin" && keyRepeat !== "") {
    // Convert the key repetition delay from Mac scale (slowest=120, fastest=2) to Linux scale (slowest=9, fastest=1000). NB: the units on Mac and Linux are multiplicative inverse.
    const keyRepeatFloat =
      (1.0 -
        (parseInt(keyRepeat) - KEY_REPEAT_RATE_MIN_MAC) /
          KEY_REPEAT_RATE_RANGE_MAC) *
        KEY_REPEAT_RATE_RANGE_LINUX +
      KEY_REPEAT_RATE_MIN_LINUX
    keyRepeat = keyRepeatFloat.toFixed()
  }

  const keyRepeatInt = parseInt(keyRepeat)
  return !isNaN(keyRepeatInt) ? keyRepeatInt : undefined
}

export { getInitialKeyRepeat, getKeyRepeat }
