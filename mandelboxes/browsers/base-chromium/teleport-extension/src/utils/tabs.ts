import pickBy from "lodash.pickby"
import { WhistTab } from "@app/constants/tabs"

const createTab = (createProperties: { url?: string; active: boolean }) => {
  if (createProperties.url === undefined) return

  if (createProperties.url.startsWith("cloud:")) {
    createProperties.url = createProperties.url.replace("cloud:", "")
  }
  return chrome.tabs.create(createProperties)
}

const updateTab = (tabId: number, updateProperties: object) =>
  chrome.tabs.update(tabId, updateProperties)

const removeTab = (tabId: number) => {
  chrome.tabs.remove([tabId])
}

const getTab = async (tabId?: number): Promise<chrome.tabs.Tab> => {
  if (tabId === undefined)
    throw new Error(`Could not get tab with undefined ID`)

  return await new Promise((resolve) => {
    chrome.tabs.get(tabId, (tab) => resolve(tab))
  })
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

const getActiveTab = async () => {
  return await new Promise<chrome.tabs.Tab>((resolve) => {
    chrome.tabs.query({ active: true, currentWindow: true }, (tabs) =>
      resolve(tabs[0])
    )
  })
}

export { createTab, updateTab, removeTab, getOpenTabs, getTab, getActiveTab }
