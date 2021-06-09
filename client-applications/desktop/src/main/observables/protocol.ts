import { combineLatest } from "rxjs"
import { map, mapTo, startWith } from "rxjs/operators"

import { fromTrigger } from "@app/utils/flows"

export const protocolIsRunning = combineLatest({
  launched: fromTrigger("childProcessSpawn").pipe(
    mapTo(true),
    startWith(false)
  ),
  closed: fromTrigger("childProcessClose").pipe(mapTo(true), startWith(false)),
}).pipe(map(({ launched, closed }) => launched && !closed))
