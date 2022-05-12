import { fromEventPattern } from "rxjs"
import { filter, map, share } from "rxjs/operators"

import { ContentScriptMessageType } from "@app/constants/messaging"

const message = fromEventPattern(
  (handler: any) => chrome.runtime.onMessage.addListener(handler),
  (handler: any) => chrome.runtime.onMessage.removeListener(handler),
  (request: any, sender: any, sendResponse: any) => ({
    request,
    sender,
    sendResponse,
  })
)

const ipcMessage = (type: ContentScriptMessageType) =>
  message.pipe(
    filter(({ request }) => request.type === type),
    map((args: { type: ContentScriptMessageType; value?: any }) => args?.value),
    share()
  )

export { ipcMessage }
