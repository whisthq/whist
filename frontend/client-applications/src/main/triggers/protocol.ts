import { fromEvent, merge } from "rxjs"
import { switchMap, map, mapTo, startWith, filter } from "rxjs/operators"
import { ChildProcess } from "child_process"

import { WhistTrigger } from "@app/constants/triggers"
import { createTrigger, fromTrigger } from "@app/main/utils/flows"
import { protocol } from "@app/main/utils/protocol"

// Observable that returns the active ChildProcess is protocol is running and undefined
// if the protocol is not running
createTrigger(
  WhistTrigger.protocol,
  fromEvent(protocol, "launched").pipe(
    switchMap((p) =>
      fromEvent(p as ChildProcess, "close").pipe(mapTo(undefined), startWith(p))
    ),
    startWith(undefined)
  )
)

// Fires whenever the protocol closes
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

// Fires whenever the protocol emits stdout
createTrigger(
  WhistTrigger.protocolStdoutData,
  fromTrigger(WhistTrigger.protocol).pipe(
    filter((p) => p !== undefined),
    switchMap((p) => fromEvent(p.stdout, "data"))
  )
)

// Fires whenever the protocol stdout ends
createTrigger(
  WhistTrigger.protocolStdoutEnd,
  fromTrigger(WhistTrigger.protocol).pipe(
    filter((p) => p !== undefined),
    switchMap((p) => fromEvent(p.stdout, "end"))
  )
)

// Observable that returns true if the protocol is successfully connected to the server
// (i.e. stream has successfully started) and false otherwise
createTrigger(
  WhistTrigger.protocolConnection,
  merge(
    fromTrigger(WhistTrigger.protocolStdoutData).pipe(
      filter(
        (data: string) => data.includes("Connected to") && data.includes("UDP")
      ),
      mapTo(true),
      startWith(false)
    ),
    fromTrigger(WhistTrigger.protocolClosed).pipe(mapTo(false))
  )
)
