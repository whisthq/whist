import { withLatestFrom } from "rxjs/operators"
import { ChildProcess } from "child_process"

import { fromTrigger } from "@app/main/utils/flows"
import { WhistTrigger } from "@app/constants/triggers"
import { pipeURLToProtocol } from "@app/main/utils/protocol"
import { MAX_URL_LENGTH, MAX_NEW_TAB_URLS } from "@app/constants/app"

fromTrigger(WhistTrigger.importTabs)
  .pipe(withLatestFrom(fromTrigger(WhistTrigger.protocol)))
  .subscribe(([payload, p]: [{ urls: string[] }, ChildProcess]) => {
    const validUrls: string[] = payload.urls.reduce(
      (filtered: string[], url: string) => {
        if (
          url.length <= MAX_URL_LENGTH &&
          filtered.length < MAX_NEW_TAB_URLS &&
          (url.startsWith("http://") ||
            url.startsWith("https://") ||
            url.startsWith("ftp://"))
        ) {
          // Escape double quotes two times
          filtered.push('\\"' + url + '\\"')
        }
        return filtered
      },
      []
    )
    pipeURLToProtocol(p, validUrls.join(" "))
  })
