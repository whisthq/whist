import { values } from "lodash"

import { logBase, LogLevel } from "@app/utils/logging"
import { fromTrigger, TriggerChannel, Trigger } from "@app/utils/flows"
import TRIGGER from "@app/utils/triggers"

let lastTimeStamp = 0

// Iterates through all the triggers and logs them
values(TRIGGER).forEach((name: string) => {
  fromTrigger(name).subscribe((x: any) => {
    logBase(name, x, LogLevel.DEBUG).catch((err) => console.error(err))

    if (name === "appReady") {
      lastTimeStamp = x?.timestamp ?? 0
      return
    }
    if (
      lastTimeStamp > 0 &&
      x.timestamp !== undefined &&
      x.timestamp > lastTimeStamp
    ) {
      logBase(
        name,
        {
          timestamp: `${(
            x.timestamp - lastTimeStamp
          ).toString()} ms since application start`,
        },
        LogLevel.DEBUG
      )
      lastTimeStamp = x.timestamp
    }
  })
})
