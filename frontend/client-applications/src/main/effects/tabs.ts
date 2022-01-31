import { withLatestFrom } from "rxjs/operators"
import { ChildProcess } from "child_process"
import Sentry from "@sentry/electron"

import { getOtherBrowserWindows } from "@app/main/utils/applescript"
import { fromTrigger } from "@app/main/utils/flows"
import { WhistTrigger } from "@app/constants/triggers"
import { pipeURLToProtocol } from "../utils/protocol"

fromTrigger(WhistTrigger.getOtherBrowserWindows).subscribe(
  (browser: string) => {
    getOtherBrowserWindows(browser).catch((err) => Sentry.captureException(err))
  }
)

fromTrigger(WhistTrigger.importTabs)
  .pipe(withLatestFrom(fromTrigger(WhistTrigger.protocol)))
  .subscribe(([payload, p]: [{ urls: string[] }, ChildProcess]) => {
    for (const url of payload.urls) {
      // A little hacky; pipeURLToProtocol doesn't work when we do multiple URLs simultaneously
      // so we add a delay in between each one
      setTimeout(() => {
        pipeURLToProtocol(p, url)
      }, payload.urls.indexOf(url) * 250)
    }
  })
