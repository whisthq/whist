import { fromEventPattern } from "rxjs"

const message = fromEventPattern(
  (handler) => chrome.runtime.onMessage.addListener(handler),
  (handler) => chrome.runtime.onMessage.removeListener(handler),
  (request, sender, sendResponse) => ({ request, sender, sendResponse })
)

export { message }
