const createTab = (createProperties: object) => {
  const promise = new Promise((resolve) => {
    chrome.tabs.create(createProperties, (tab: chrome.tabs.Tab) => {
      resolve(tab)
    })
  })

  return promise
}

const activateTab = (tabId: number) => {
  chrome.tabs.update(tabId, { active: true })
}

const removeTab = (tabId: number) => {
  chrome.tabs.remove([tabId])
}

export { createTab, activateTab, removeTab }
