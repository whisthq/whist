import {
  DEFAULT_LINUX_USER_AGENT,
  DEFAULT_MAC_USER_AGENT,
  DEFAULT_WINDOWS_USER_AGENT,
} from "@app/constants/app"

const getUserAgent = () => {
  if (process.platform === "linux") {
    return DEFAULT_LINUX_USER_AGENT
  } else if (process.platform === "darwin") {
    return DEFAULT_MAC_USER_AGENT
  } else {
    return DEFAULT_WINDOWS_USER_AGENT
  }
}

export { getUserAgent }
