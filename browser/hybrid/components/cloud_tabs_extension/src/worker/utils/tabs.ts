import find from "lodash.find"
import remove from "lodash.remove"

import { whistState } from "@app/worker/utils/state"

const getActiveTab = async (windowId?: number): Promise<chrome.tabs.Tab> => {
  const tabPromise = new Promise<chrome.tabs.Tab>((resolve) => {
    chrome.tabs.query(
      {
        active: true,
        ...(windowId !== undefined ? { windowId } : { currentWindow: true }),
      },
      (tabs) => {
        resolve(tabs[0])
      }
    )
  })

  return await tabPromise
}

const getTab = async (tabId?: number): Promise<chrome.tabs.Tab> => {
  if (tabId === undefined)
    throw new Error(`Could not get tab with undefined ID`)

  return await new Promise((resolve) => {
    chrome.tabs.get(tabId, (tab) => resolve(tab))
  })
}

const updateTabUrl = (tabId?: number, url?: string, callback?: () => void) => {
  if (tabId === undefined)
    throw new Error(`Could not update tab because ID is undefined`)
  if (url === undefined)
    throw new Error(`Could not update tab ${tabId} because URL is undefined`)

  return callback === undefined
    ? chrome.tabs.update(tabId, { url })
    : chrome.tabs.update(tabId, { url }, callback)
}

const queryTabs = async (queryInfo: object): Promise<chrome.tabs.Tab[]> => {
  return await new Promise((resolve) => {
    chrome.tabs.query(queryInfo, (tabs) => resolve(tabs))
  })
}

const isCloudTab = (tab?: chrome.tabs.Tab) =>
  tab?.url?.startsWith("cloud:") ?? false

const isActiveCloudTab = (tab: chrome.tabs.Tab) => {
  const exists = find(whistState.activeCloudTabs, (el) => el.id === tab.id)
  return exists !== undefined
}

const stripCloudUrl = (url: string) => url.replace("cloud:", "")

const getNumberOfCloudTabs = async () => {
  const tabPromise = new Promise<number>((resolve) => {
    chrome.tabs.query({ url: "cloud:*" }, (tabs) => {
      resolve(tabs.length)
    })
  })

  return await tabPromise
}

const convertToCloudTab = (tab: chrome.tabs.Tab) => {
  if (!isCloudTab(tab) && tab.url !== undefined)
    updateTabUrl(tab.id, `cloud:${tab.url}`)
}

const addTabToQueue = (tab: chrome.tabs.Tab) => {
  const exists = find(whistState.waitingCloudTabs, (el) => el.id === tab.id)
  if (exists === undefined) whistState.waitingCloudTabs.push(tab)
}

const removeTabFromQueue = (tab: chrome.tabs.Tab) => {
  remove(whistState.waitingCloudTabs, { id: tab.id })
}

const markActiveCloudTab = (tab: chrome.tabs.Tab) => {
  const exists = find(whistState.activeCloudTabs, (el) => el.id === tab.id)
  if (exists === undefined) whistState.activeCloudTabs.push(tab)
}

const unmarkActiveCloudTab = (tab: chrome.tabs.Tab) => {
  remove(whistState.activeCloudTabs, { id: tab.id })
}

export {
  getActiveTab,
  getTab,
  updateTabUrl,
  queryTabs,
  isCloudTab,
  isActiveCloudTab,
  stripCloudUrl,
  getNumberOfCloudTabs,
  addTabToQueue,
  removeTabFromQueue,
  convertToCloudTab,
  markActiveCloudTab,
  unmarkActiveCloudTab,
}
