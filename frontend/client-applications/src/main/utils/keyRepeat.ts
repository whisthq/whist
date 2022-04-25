import { execCommand } from "@app/main/utils/execCommand"
import {
  INITIAL_KEY_REPEAT_MAC_TO_LINUX_CONVERSION_FACTOR,
  MIN_KEY_REPEAT_MAC,
  KEY_REPEAT_RANGE_LINUX,
  KEY_REPEAT_RANGE_MAC,
  MIN_KEY_REPEAT_LINUX,
  INITIAL_REPEAT_COMMAND_MAC,
  INITIAL_REPEAT_COMMAND_LINUX,
  REPEAT_COMMAND_MAC,
  REPEAT_COMMAND_LINUX,
  MIN_KEY_REPEAT_WINDOWS,
  MAX_KEY_REPEAT_WINDOWS,
  MIN_INTIAL_REPEAT_WINDOWS,
} from "@app/constants/keyRepeat"
import { windowsKey } from "@app/main/utils/windowsRegistry"

const windowsInitialRepeatRaw = () => {
  /*
  Returns the raw Windows initial keyboard repeat rate as stored in the Windows Registry

  Returns:
   initialRepeat (int): The initial keyboard repeat rate
*/
  if (process.platform !== "win32")
    throw new Error(`process.platform ${process.platform} is not win32`)

  const initialRepeat =
    windowsKey("Control Panel\\Keyboard").getValue("KeyboardDelay") ?? undefined
  if (initialRepeat === undefined) return MIN_INTIAL_REPEAT_WINDOWS

  return parseInt(initialRepeat)
}

const windowsRepeatRaw = () => {
  /*
  Returns the raw Windows keyboard repeat rate as stored in the Windows Registry

  Returns:
   initialRepeat (int): The keyboard repeat rate
*/
  if (process.platform !== "win32")
    throw new Error(`process.platform ${process.platform} is not win32`)

  const repeat =
    windowsKey("Control Panel\\Keyboard").getValue("KeyboardSpeed") ?? undefined
  if (repeat === undefined) return MAX_KEY_REPEAT_WINDOWS

  return parseInt(repeat)
}

const macInitialRepeatRaw = () => {
  /*
  Returns the raw Mac initial keyboard repeat rate

  Returns:
   initialRepeat (int): The initial keyboard repeat rate
*/
  if (process.platform !== "darwin")
    throw new Error(`process.platform ${process.platform} is not darwin`)

  return parseInt(
    execCommand(INITIAL_REPEAT_COMMAND_MAC, ".", {}, "pipe")
      ?.toString()
      ?.replace(/\n$/, "") ?? ""
  )
}

const macRepeatRaw = () => {
  /*
  Returns the raw Mac keyboard repeat rate

  Returns:
   repeat (int): The keyboard repeat rate
*/
  if (process.platform !== "darwin")
    throw new Error(`process.platform ${process.platform} is not darwin`)

  return parseInt(
    execCommand(REPEAT_COMMAND_MAC, ".", {}, "pipe")
      ?.toString()
      ?.replace(/\n$/, "") ?? ""
  )
}

const linuxInitialRepeatRaw = () => {
  /*
  Returns the raw Linux initial keyboard repeat rate

  Returns:
   initialRepeat (int): The initial keyboard repeat rate
*/
  if (process.platform !== "linux")
    throw new Error(`process.platform ${process.platform} is not linux`)

  let initialRepeat =
    execCommand(INITIAL_REPEAT_COMMAND_LINUX, ".", {}, "pipe")
      ?.toString()
      ?.replace(/\n$/, "") ?? ""

  // Filter out strings in initial repeat
  initialRepeat = initialRepeat.substring(
    initialRepeat.indexOf("auto repeat delay:") +
      "auto repeat delay:".length +
      2,
    initialRepeat.indexOf("repeat rate:") - 4
  )

  return parseInt(initialRepeat)
}

const linuxRepeatRaw = () => {
  /*
  Returns the raw Linux initial keyboard repeat rate

  Returns:
   initialRepeat (int): The initial keyboard repeat rate
*/
  if (process.platform !== "linux")
    throw new Error(`process.platform ${process.platform} is not linux`)

  let repeat =
    execCommand(REPEAT_COMMAND_LINUX, ".", {}, "pipe")
      ?.toString()
      ?.replace(/\n$/, "") ?? ""

  // Filter out strings in repeat
  repeat = repeat.substring(
    repeat.indexOf("repeat rate:") + "repeat rate:".length + 2,
    repeat.length
  )

  return parseInt(repeat)
}

const macRepeatToLinux = (repeat: number) =>
  /*
  Maps the Mac key repeat to the Linux key repeat

  Returns:
   repeat (int): The mapped repeat rate
*/
  Math.round(
    (1.0 - (repeat - MIN_KEY_REPEAT_MAC) / KEY_REPEAT_RANGE_MAC) *
      KEY_REPEAT_RANGE_LINUX +
      MIN_KEY_REPEAT_LINUX
  )

const macInitialRepeatToLinux = (initialRepeat: number) =>
  /*
  Maps the Mac initial key repeat to the Linux initial key repeat

  Returns:
    initialRepeat (int): The mapped initial repeat rate
*/
  Math.round(initialRepeat * INITIAL_KEY_REPEAT_MAC_TO_LINUX_CONVERSION_FACTOR)

const windowsRepeatToLinux = (repeat: number) =>
  /*
  Maps the Mac key repeat to the Linux key repeat

  Returns:
   repeat (int): The mapped repeat rate
*/
  Math.round(Math.max(MIN_KEY_REPEAT_WINDOWS, repeat - 1))

const windowsInitialRepeatToLinux = (initialRepeat: number) =>
  Math.max(300 - 100 * initialRepeat, 0)

const getKeyRepeat = () => {
  switch (process.platform) {
    case "darwin":
      return macRepeatToLinux(macRepeatRaw())
    case "linux":
      return linuxRepeatRaw()
    case "win32":
      return windowsRepeatToLinux(windowsRepeatRaw())
    default:
      return 0
  }
}

const getInitialKeyRepeat = () => {
  switch (process.platform) {
    case "darwin":
      return macInitialRepeatToLinux(macInitialRepeatRaw())
    case "linux":
      return linuxInitialRepeatRaw()
    case "win32":
      return windowsInitialRepeatToLinux(windowsInitialRepeatRaw())
    default:
      return 0
  }
}

export { getInitialKeyRepeat, getKeyRepeat }
