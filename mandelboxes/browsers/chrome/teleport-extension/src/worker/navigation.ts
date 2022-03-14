import { WELCOME_URL } from "@app/constants/pages"

const initChromeWelcomeRedirect = () => {
  chrome.webNavigation.onBeforeNavigate.addListener((details) => {
    if (details.url === "chrome://welcome/") {
      chrome.tabs.update({ url: WELCOME_URL })
    }
  })
}

export { initChromeWelcomeRedirect }
