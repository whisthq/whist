const createAuthTab = () => {
  chrome.tabs.create({
    url: chrome.runtime.getURL("src/tabs.html?show=auth"),
  })
}

export { createAuthTab }
