import { Observable } from "rxjs"
import { filter, share, map } from "rxjs/operators"
import { mapValues } from "lodash"

import { FlowReturnType, FlowEvent } from "@app/@types/state"

export const fork = <T>(
  source: Observable<T>,
  filters: { [gate: string]: (result: T) => boolean }
) => {
  const shared = source.pipe(share())
  return mapValues(filters, (fn) => shared.pipe(filter(fn)))
}

export const flow = <A>(
  name: string,
  fn: (
    childName: string,
    trigger: Observable<A>
  ) => { [key: string]: Observable<any> }
) => (childName: string, trigger: Observable<A>): FlowReturnType =>
  mapValues(fn(`${name}.${childName}`, trigger), (obs) =>
    obs.pipe(
      map((x) => ({name: `${name}.${childName}`, payload: x} as FlowEvent)),
      share()
    )
  )

export const withEffects = (flow: FlowReturnType, fn: (args: any) => void) => {
  flow.success.subscribe(
    (x: FlowEvent) => {
      fn(x.payload)
    }
  )

  return flow
}