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

export { createTab, activateTab, removeTab }
