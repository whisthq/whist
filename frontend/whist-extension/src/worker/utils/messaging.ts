import { fromEventPattern } from "rxjs"

const message = fromEventPattern(
  (handler: any) => chrome.runtime.onMessage.addListener(handler),
  (handler: any) => chrome.runtime.onMessage.removeListener(handler),
  (request: any, sender: any, sendResponse: any) => ({
    request,
    sender,
    sendResponse,
  })
)

export { message }
