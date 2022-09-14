import { getTabId } from "./session"

const initializeGetTabId = () => {
  const promise = new Promise<void>((resolve) => {
    chrome.tabs.getCurrent((tab) => {
      if (tab?.id !== undefined) sessionStorage.setNumber("tabId", tab.id)
      if (tab?.windowId !== undefined) sessionStorage.setNumber("windowId", tab.windowId)
      resolve()
    })
  })

  return promise
}

const initializeFaviconUpdater = () => {
  ;(chrome as any).whist.onMessage.addListener((message: string) => {
    const parsed = JSON.parse(message)

    if (parsed.type !== "UPDATE_TAB") return
    if (parsed?.value?.favIconUrl === undefined) return
    if (parsed?.value?.id !== getTabId()) return

    if (parsed?.value?.favIconUrl !== undefined) {
      let link: any = document.querySelector("link[rel~='icon']")
      if (!link) {
        link = document.createElement("link")
        link.rel = "icon"
        document.getElementsByTagName("head")[0].appendChild(link)
      }
      link.href = parsed.value.favIconUrl
    }
  })
}

const initializeTitleUpdater = () => {
  // Set the initial title
  const domain = new URL(document.URL.replace("cloud:", ""))?.hostname
    ?.replace("www.", "")
    ?.split(".")[0]
  document.title = domain.charAt(0).toUpperCase() + domain.slice(1)

  // Update the title
  ;(chrome as any).whist.onMessage.addListener((message: string) => {
    const parsed = JSON.parse(message)

    if (parsed.type !== "UPDATE_TAB") return
    if (parsed?.value?.title === undefined) return
    if (parsed?.value?.id !== getTabId()) return

    document.title = parsed.value.title
  })
}

export { initializeGetTabId, initializeFaviconUpdater, initializeTitleUpdater }
