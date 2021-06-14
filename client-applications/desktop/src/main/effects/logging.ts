import { values } from "lodash"

import { logBase, LogLevel } from "@app/utils/logging"
import { fromTrigger } from "@app/utils/flows"
import TRIGGER from "@app/utils/triggers"

// Iterates through all the triggers and logs them
values(TRIGGER).forEach((name: string) => {
  fromTrigger(name).subscribe((x: object) => {
    logBase(name, x, LogLevel.DEBUG).catch((err) => console.error(err))
  })
})
