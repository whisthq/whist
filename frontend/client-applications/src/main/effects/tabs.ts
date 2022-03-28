import { withLatestFrom } from "rxjs/operators"
import { ChildProcess } from "child_process"

import { fromTrigger } from "@app/main/utils/flows"
import { WhistTrigger } from "@app/constants/triggers"
import { pipeURLToProtocol } from "@app/main/utils/protocol"
import { MAX_URL_LENGTH, MAX_NEW_TAB_URLS } from "@app/constants/app"

fromTrigger(WhistTrigger.importTabs)
  .pipe(withLatestFrom(fromTrigger(WhistTrigger.protocol)))
  .subscribe(([payload, p]: [{ urls: string[] }, ChildProcess]) => {
    const validUrls: string[] = []
    for (const url of payload.urls) {
      if (validUrls.length === MAX_NEW_TAB_URLS) {
        break
      } else if (
        url.length <= MAX_URL_LENGTH &&
        (url.startsWith("http://") || url.startsWith("https://"))
      ) {
        // Pass only the valid urls to the protocol
        validUrls.push(url)
      }
    }
    pipeURLToProtocol(p, validUrls.join("|"))
  })
