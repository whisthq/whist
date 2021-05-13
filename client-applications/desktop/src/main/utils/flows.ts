import { Observable, Subject } from "rxjs"
import { filter, share, map, tap } from "rxjs/operators"
import { mapValues, values, toPairs } from "lodash"

export type Trigger = {
  name: string
  payload: any
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

export const flow = <A>(
  name: string,
  fn: (trigger: Observable<A>) => { [key: string]: Observable<any> }
) => (trigger: Observable<A>) =>
  mapValues(fn(trigger), (obs) =>
    obs.pipe(
      tap(() => console.log(name)),
      share()
    )
  )

export const trigger = <A>(name: string, obs: Observable<A>) =>
  obs.pipe(
    tap(() => console.log("Trigger created for ", name)),
    // Pipe to the TriggerChannel
    tap((x: A) =>
      TriggerChannel.next({ name: `${name}`, payload: x } as Trigger)
    ),
    share()
  )

export const fromTrigger = (name: string): Observable<any> =>
  TriggerChannel.pipe(
    // Filter out triggers by name. Note this allows for partial, case-insensitive string matching,
    // so filtering for "failure" will emit every time any trigger with "failure" in the name fires.
    filter((x: Trigger) => x.name.toLowerCase().includes(name.toLowerCase())),
    // Flatten the trigger so that it can be consumed by a subscriber without transforms
    map((x: Trigger) => ({ ...x.payload, name: x.name }))
  )
