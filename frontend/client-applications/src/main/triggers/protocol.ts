import { fromEvent, merge } from "rxjs"
import { switchMap, map, mapTo, startWith, filter } from "rxjs/operators"
import { ChildProcess } from "child_process"

import { WhistTrigger } from "@app/constants/triggers"
import { createTrigger, fromTrigger } from "@app/main/utils/flows"
import { protocol } from "@app/main/utils/protocol"

createTrigger(
  WhistTrigger.protocol,
  fromEvent(protocol, "launched").pipe(
    switchMap((p) =>
      fromEvent(p as ChildProcess, "close").pipe(mapTo(undefined), startWith(p))
    ),
    startWith(undefined)
  )
)

createTrigger(
  WhistTrigger.protocolClosed,
  fromEvent(protocol, "launched").pipe(
    switchMap((p) =>
      fromEvent(p as ChildProcess, "close").pipe(
        map((code) => ({ crashed: (code ?? 0) === 1 }))
      )
    )
  )
)

createTrigger(
  WhistTrigger.protocolStdoutData,
  fromTrigger(WhistTrigger.protocol).pipe(
    filter((p) => p !== undefined),
    switchMap((p) => fromEvent(p.stdout, "data"))
  )
)

createTrigger(
  WhistTrigger.protocolStdoutEnd,
  fromTrigger(WhistTrigger.protocol).pipe(
    filter((p) => p !== undefined),
    switchMap((p) => fromEvent(p.stdout, "end"))
  )
)

createTrigger(
  WhistTrigger.protocolConnection,
  merge(
    fromTrigger(WhistTrigger.protocolStdoutData).pipe(
      filter(
        (data: string) => data.includes("Connected on") && data.includes("tcp")
      ),
      mapTo(true),
      startWith(false)
    ),
    fromTrigger(WhistTrigger.protocolClosed).pipe(mapTo(false))
  )
)
