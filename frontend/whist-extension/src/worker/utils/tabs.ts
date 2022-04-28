import { authTab, loggedInTab } from "@app/constants/tabs"

const createAuthTab = () => {
  chrome.tabs.create({
    url: chrome.runtime.getURL(`src/tabs.html?show=${authTab}`),
  })
}

const createLoggedInTab = () => {
  chrome.tabs.create({
    url: chrome.runtime.getURL(`src/tabs.html?show=${loggedInTab}`),
  })
}

export { createAuthTab, createLoggedInTab }
