import { values, omit } from "lodash"

import { logBase, LogLevel } from "@app/utils/logging"
import { fromTrigger } from "@app/utils/flows"
import TRIGGER from "@app/utils/triggers"

// This is a proxy for when the application started in UNIX (ms since epoch). It's not perfect, but
// should be good enough.
const startTime = Date.now()

// Iterates through all the triggers and logs them
values(TRIGGER).forEach((name: string) => {
  fromTrigger(name).subscribe((x: any) => {
    if (x.timestamp !== undefined && x.timestamp > startTime) {
      logBase(
        name,
        { ...omit(x, ["timestamp"]), msSinceStart: x.timestamp - startTime },
        LogLevel.DEBUG
      ).catch((err) => console.error(err))
    } else {
      logBase(name, omit(x, ["timestamp"]), LogLevel.DEBUG).catch((err) =>
        console.error(err)
      )
    }
  })
})
