import { fromEventPattern } from "rxjs"
import { filter } from "rxjs/operators"

const message = fromEventPattern(
  (handler: any) => chrome.runtime.onMessage.addListener(handler),
  (handler: any) => chrome.runtime.onMessage.removeListener(handler),
  (request: any, sender: any, sendResponse: any) => ({
    request,
    sender,
    sendResponse,
  })
)

const ipcMessage = (name: string) =>
  message.pipe(filter(({ request }) => request === name))

export { ipcMessage }
