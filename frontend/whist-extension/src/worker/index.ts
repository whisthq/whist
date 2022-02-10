import { authPortalURL } from "./utils/auth"

chrome.identity.launchWebAuthFlow(
  {
    url: authPortalURL(),
    interactive: true,
  },
  (callbackUrl) => {
    console.log("done", callbackUrl)
  }
)
