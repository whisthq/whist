import { values, omit } from "lodash"
import { zip } from "rxjs"

import { logBase, LogLevel } from "@app/utils/logging"
import { fromTrigger } from "@app/utils/flows"
import TRIGGER from "@app/utils/triggers"

// This is a proxy for when the application started in UNIX (ms since epoch). It's not perfect, but
// should be good enough.
const startTime = Date.now()

const logTime = (x: { timestamp: number }, start: number) => {
    if (x.timestamp > startTime) {
        logBase(
            "appReadyTime",
            {
                ms: x.timestamp - startTime,
            },
            LogLevel.DEBUG
        ).catch((err) => console.error(err))
    }
}

// Iterates through all the triggers and logs them
values(TRIGGER).forEach((name: string) => {
    fromTrigger(name).subscribe((x: any) => {
        logBase(name, omit(x, ["timestamp"]), LogLevel.DEBUG).catch((err) =>
            console.error(err)
        )
    })
})

fromTrigger("appReady").subscribe((x: any) => logTime(x, startTime))

zip([
    fromTrigger("authFlowSuccess"),
    fromTrigger("mandelboxFlowSuccess"),
]).subscribe(([a, m]: [{ timestamp: number }, { timestamp: number }]) =>
    logTime(a, m.timestamp)
)
