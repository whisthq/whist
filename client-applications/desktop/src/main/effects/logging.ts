import { values, omit } from "lodash"
import { zip, Observable, of } from "rxjs"

import { logBase, LogLevel } from "@app/utils/logging"
import { fromTrigger } from "@app/utils/flows"
import TRIGGER from "@app/utils/triggers"

// This is a proxy for when the application started in UNIX (ms since epoch). It's not perfect, but
// should be good enough.
const startTime = Date.now()

// helper function that takes two observables with timestamps and logs the diff
const logTime = (
    name: string,
    firstObs: Observable<{ timestamp: number }>,
    secondObs: Observable<{ timestamp: number }>
) => {
    zip([firstObs, secondObs]).subscribe(
        ([f, s]: [{ timestamp: number }, { timestamp: number }]) => {
            if (s.timestamp > f.timestamp) {
                logBase(
                    name,
                    {
                        ms: s.timestamp - f.timestamp,
                    },
                    LogLevel.DEBUG
                ).catch((err) => console.error(err))
            }
        }
    )
}

// Iterates through all the triggers and logs them
values(TRIGGER).forEach((name: string) => {
    fromTrigger(name).subscribe((x: any) => {
        logBase(name, omit(x, ["timestamp"]), LogLevel.DEBUG).catch((err) =>
            console.error(err)
        )
    })
})

// Log the time diffs (ms) between certain triggers to Amplitude
logTime(
    "electronLoadTime",
    of({ timestamp: startTime }),
    fromTrigger("appReady")
)
logTime(
    "mandelboxCreateTime",
    fromTrigger("authFlowSuccess"),
    fromTrigger("mandelboxFlowSuccess")
)
