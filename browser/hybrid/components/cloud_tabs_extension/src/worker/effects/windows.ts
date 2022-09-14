import { serverWindowCreated } from "@app/worker/events/windows"

serverWindowCreated.subscribe(
  ([window]: [chrome.windows.Window & { url: string }]) => {
    chrome.windows.create({
      url: window.url,
      incognito: window.incognito,
      state: window.state,
      type: window.type,
    } as any)
  }
)
