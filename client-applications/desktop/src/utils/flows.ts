import { Observable, Subject } from "rxjs"
import { filter, share, map, tap } from "rxjs/operators"
import { mapValues, values, toPairs } from "lodash"

export type Trigger = {
  name: string
  payload: Record<string, any>
}

export type FlowOutput = {
  [key: string]: Observable<Trigger>
}

export const TriggerChannel = new Subject<Trigger>()

export const fork = <T>(
  source: Observable<T>,
  filters: { [gate: string]: (result: T) => boolean }
) => {
  const shared = source.pipe(share())
  return mapValues(filters, (fn) => shared.pipe(filter(fn)))
}

// TODO: flow() and trigger() look pretty similar, can we combine them?
export const flow = (
  name: string,
  fn: (trigger: Observable<Trigger>) => { [key: string]: Observable<any> }
) => (trigger: Observable<Trigger>): FlowOutput =>
  mapValues(fn(trigger.payload), (obs, key) =>
    obs.pipe(
      // Map success/failure/etc. to an Observable<Trigger>
      tap((x) =>
        TriggerChannel.next({
          name: `${name}${key.charAt(0).toUpperCase() + key.slice(1)}`,
          payload: x,
        } as Trigger)
      ),
      share()
    )
  )

export const trigger = <A>(name: string, obs: Observable<A>): Observable<Trigger> =>
  obs.pipe(
    // Pipe to the TriggerChannel
    tap((x: Trigger) => TriggerChannel.next({ name: `${name}`, payload: x })),
    share()
  )

export const fromTrigger = (name: string, status?: string) =>
  TriggerChannel.pipe(
    filter((x: Trigger) => x.name === name),
    map((x: Trigger) => x.payload)
  )