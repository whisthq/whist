import { authPortalURL } from "@whist/core-ts"

chrome.tabs.create({ url: chrome.runtime.getURL(authPortalURL()) })
