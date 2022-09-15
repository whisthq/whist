import { downloadStarted } from "@app/worker/events/download"

downloadStarted.subscribe(([url]: [string]) => {
  void chrome.downloads.download({ url })
})
