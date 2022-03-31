/*
  This entire file is injected as a script into the current window's DOM.
*/

import {
  URLS_THAT_NEED_LINUX_USERAGENT,
  LINUX_APP_VERSION,
  LINUX_USER_AGENT,
  LINUX_PLATFORM,
} from "@app/constants/urls"

var navigator = window.navigator
const currentURL = document.URL

if (URLS_THAT_NEED_LINUX_USERAGENT.some((url) => currentURL.includes(url))) {
  Object.defineProperty(navigator, "userAgent", {
    get: () => {
      return LINUX_USER_AGENT
    },
  })

  Object.defineProperty(navigator, "appVersion", {
    get: () => {
      return LINUX_APP_VERSION
    },
  })

  Object.defineProperty(navigator, "platform", {
    get: () => {
      return LINUX_PLATFORM
    },
  })
}
