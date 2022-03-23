import { WELCOME_URL } from "@app/constants/pages"

const redirectIfWelcome = (tab: chrome.tabs.Tab) => {
  if (tab.url !== "chrome://welcome/" && tab.pendingUrl !== "chrome://welcome/")
    return

  if (tab.id === undefined) {
    chrome.tabs.update({ url: WELCOME_URL })
  } else {
    chrome.tabs.update(tab.id, { url: WELCOME_URL })
  }
}

const initChromeWelcomeRedirect = () => {
  chrome.tabs.query({ currentWindow: true }, (tabs) => {
    tabs.forEach((tab) => {
      redirectIfWelcome(tab)
    })
  })

  chrome.tabs.onUpdated.addListener((_tabId, _changeInfo, tab) => {
    redirectIfWelcome(tab)
  })
}

export { initChromeWelcomeRedirect }
