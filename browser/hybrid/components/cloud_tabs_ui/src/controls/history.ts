import { getTabId } from "./session"

let currentHistoryIndex = 0

const initializePopStateHandler = () => {
  window.addEventListener("popstate", () => {
    ;(chrome as any).whist.broadcastWhistMessage(
      JSON.stringify({
        type: "NAVIGATION",
        value: {
          id: getTabId(),
          url: document.URL,
        },
      })
    )

    currentHistoryIndex = window.history.state.index
  })
}

const initializeURLUpdater = () => {
  let webuiJustOpened = true
  let serverHistoryLength = 0
  let maxHistoryIndex = 0

  ;(chrome as any).whist.onMessage.addListener((message: string) => {
    const parsed = JSON.parse(message)

    // If it's not a tab update, ignore
    if (parsed.type !== "UPDATE_TAB") return
    // If there's no URL change, ignore
    if (parsed?.value?.url === undefined) return
    // If it's for a different cloud tab, ignore
    if (parsed?.value?.id !== getTabId()) return

    // If the web UI was just opened, we know we're at history index 0 so we set the history state accordingly
    if (webuiJustOpened) {
      webuiJustOpened = false
      window.history.replaceState(
        { index: 0, urls: [parsed.value.url] },
        "",
        parsed.value.url
      )
      // History replaceStates can only happen if we're at the end of history. So we check to see if we're the end of
      // server history; if we are and the URL is changing but the server-side history length is the same,
      // then the server must be doing a replaceState. We store the new replaced URL in window.history.state.
    } else if (
      parsed.value.historyLength === serverHistoryLength &&
      maxHistoryIndex === currentHistoryIndex &&
      parsed.value.url !== document.URL
    ) {
      window.history.replaceState(
        {
          index: window.history.state.index,
          urls: [...(window.history.state.urls ?? []), parsed.value.url],
        },
        "",
        document.URL
      )
      // If the new URL is not in our list of previous URLs at this history index, then the server must be doing a pushState
    } else if (
      document.URL !== parsed.value.url &&
      !window.history.state.urls.includes(parsed.value.url)
    ) {
      window.history.pushState(
        { index: currentHistoryIndex + 1, urls: [parsed.value.url] },
        "",
        parsed.value.url
      )
      maxHistoryIndex = currentHistoryIndex + 1
      currentHistoryIndex = maxHistoryIndex
    }

    serverHistoryLength = parsed.value.historyLength
  })
}

export { initializeURLUpdater, initializePopStateHandler }
