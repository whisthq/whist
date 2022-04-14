const runInActiveTab = (fn: (tabID: number) => void) => {
  chrome.tabs.query(
    { active: true, currentWindow: true },
    (tabs: chrome.tabs.Tab[]) => {
      if (tabs[0].id !== undefined) fn(tabs[0].id)
    }
  )
}

export { runInActiveTab }
