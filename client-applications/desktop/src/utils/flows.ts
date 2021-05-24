import { Observable, ReplaySubject } from "rxjs"
import { filter, share, tap, map } from "rxjs/operators"
import { mapValues } from "lodash"
import { withMocking } from "@app/main/testing"

// A Trigger is emitted by an Observable. Every Trigger has a name and payload.
export interface Trigger {
  name: string
  payload: any
}

// This is the observable that emits Triggers.
export const TriggerChannel = new ReplaySubject<Trigger>()

export const fork = <T>(
  source: Observable<T>,
  filters: { [gate: string]: (result: T) => boolean }
): { [gate: string]: Observable<T> } => {
  const shared = source.pipe(share())
  return mapValues(filters, (fn) => shared.pipe(filter(fn)))
}

export const flow =
  <T>(
    name: string,
    fn: (t: Observable<T>) => { [key: string]: Observable<any> }
  ): ((t: Observable<T>) => { [key: string]: Observable<any> }) =>
  (trigger: Observable<T>) => {
    const channels = fn(trigger)

    return mapValues(withMocking(name, trigger, channels), (obs, key) =>
      obs.pipe(share())
    )
  }

export const createTrigger = <A>(name: string, obs: Observable<A>) => {
  obs.subscribe((x: A) => {
    TriggerChannel.next({ name: `${name}`, payload: x } as Trigger)
  })

  return obs
}

export const fromTrigger = (name: string): Observable<any> => {
  return TriggerChannel.pipe(
    // Filter out triggers by name. Note this allows for partial, case-insensitive string matching,
    // so filtering for "failure" will emit every time any trigger with "failure" in the name fires.
    filter((x: Trigger) => x.name === name),
    // Flatten the trigger so that it can be consumed by a subscriber without transforms
    map((x: Trigger) => x.payload)
  )
}
