import { AUTH_TAB, LOGGED_IN_TAB } from "@app/constants/tabs"

const createAuthTab = () => {
  chrome.tabs.create({
    url: chrome.runtime.getURL(`src/tabs.html?show=${AUTH_TAB}`),
  })
}

const createLoggedInTab = () => {
  chrome.tabs.create({
    url: chrome.runtime.getURL(`src/tabs.html?show=${LOGGED_IN_TAB}`),
  })
}

export { createAuthTab, createLoggedInTab }
