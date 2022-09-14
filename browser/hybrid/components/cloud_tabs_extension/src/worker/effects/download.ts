import { downloadStarted } from "@app/worker/events/download"

downloadStarted.subscribe(([url]: [string]) => {
  chrome.downloads.download({ url })
})
