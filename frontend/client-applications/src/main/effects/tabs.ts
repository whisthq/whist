import { withLatestFrom } from "rxjs/operators"
import { ChildProcess } from "child_process"

import { fromTrigger } from "@app/main/utils/flows"
import { WhistTrigger } from "@app/constants/triggers"
import { pipeURLToProtocol } from "@app/main/utils/protocol"

fromTrigger(WhistTrigger.importTabs)
  .pipe(withLatestFrom(fromTrigger(WhistTrigger.protocol)))
  .subscribe(([payload, p]: [{ urls: string[] }, ChildProcess]) => {
    const imported_urls = payload.urls.join("|")
    console.log(imported_urls)
    pipeURLToProtocol(p, imported_urls)
  })
