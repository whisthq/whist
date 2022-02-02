import { withLatestFrom } from "rxjs/operators"
import { ChildProcess } from "child_process"

import { fromTrigger } from "@app/main/utils/flows"
import { WhistTrigger } from "@app/constants/triggers"
import { pipeURLToProtocol } from "@app/main/utils/protocol"

fromTrigger(WhistTrigger.importTabs)
  .pipe(withLatestFrom(fromTrigger(WhistTrigger.protocol)))
  .subscribe(([payload, p]: [{ urls: string[] }, ChildProcess]) => {
    for (const url of payload.urls) {
      // Tech debt: pipeURLToProtocol doesn't work when we do multiple URLs simultaneously
      // so we add a delay in between each one; the server also seems to crash if we make the loop
      // too tight so we do 1.5 seconds between each URL to be safe
      setTimeout(() => {
        pipeURLToProtocol(p, url)
      }, payload.urls.indexOf(url) * 5000)
    }
  })
