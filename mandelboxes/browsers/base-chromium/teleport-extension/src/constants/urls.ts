export const LINUX_USER_AGENT =
  "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/99.0.4844.82 Safari/537.36"

export const LINUX_APP_VERSION =
  "5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/99.0.4844.82 Safari/537.36"

export const LINUX_PLATFORM = "Linux x86_64"

// We want websites that make use of the Cmd + [Key] Mac keyboard shortcut to have Linux user agents in order
// for those shortcuts to work correctly, since the protocol maps Cmd to Ctrl
export const URLS_THAT_NEED_LINUX_USERAGENT = ["figma.com"]

export const URLS_THAT_NEED_CAMERA_MIC = [
  "meet.google.com",
  "app.gather.town",
  "around.co",
  "discord.com",
  "slack.com",
  "zoom.us",
]

export const URLS_NO_OVERSCROLL = ["youtube.com", "figma.com"]
