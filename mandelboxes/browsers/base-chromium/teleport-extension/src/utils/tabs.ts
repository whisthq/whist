const createTab = (createProperties: { url?: string; active: boolean }) => {
  const promise = new Promise((resolve) => {
    if (createProperties.url === undefined) return

    if (createProperties.url.startsWith("cloud:")) {
      createProperties.url = createProperties.url.replace("cloud:", "")
    }
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
