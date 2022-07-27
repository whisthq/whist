import pickBy from "lodash.pickby"
import { WhistTab } from "@app/constants/tabs"

const createTab = (
  createProperties: { url?: string; active: boolean },
  callback?: (tab: chrome.tabs.Tab) => void
) => {
  if (createProperties.url === undefined) return

  if (createProperties.url.startsWith("cloud:")) {
    createProperties.url = createProperties.url.replace("cloud:", "")
  }
  chrome.tabs.create(createProperties, (tab: chrome.tabs.Tab) => {
    if (callback !== undefined) callback(tab)
  })
}

const activateTab = (tabId: number) => {
  chrome.tabs.update(tabId, { active: true })
}

const removeTab = (tabId: number) => {
  chrome.tabs.remove([tabId])
}

const getOpenTabs = async (): Promise<Array<WhistTab>> => {
  const promise = new Promise<any | undefined>((resolve) => {
    chrome.storage.local.get(null, (result) => resolve(result))
  })
  let result = await promise
  if (result !== undefined) {
    result = pickBy(result, (_value, key) => {
      return !isNaN(Number(key))
    })
    if (result !== undefined)
      result = Object.keys(result).map((key) => ({
        clientTabId: Number(key),
        tab: result[key],
      }))
  }
  return result ?? []
}

export { createTab, activateTab, removeTab, getOpenTabs }
