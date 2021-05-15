import { Observable, Subject } from "rxjs"
import { filter, share, tap, map } from "rxjs/operators"
import { mapValues, truncate, values, toPairs } from "lodash"
import stringify from "json-stringify-safe"


export type Trigger = {
  name: string
  payload: any
}

export type FlowOutput = {
  [key: string]: Observable<Trigger>
}

export const TriggerChannel = new Subject<Trigger>()

const logFormat = (...args: any[]) => {
  let [title, message, value] = args
  if (value === undefined) {
    value = message
    message = undefined
  }
  if (value === undefined && message === undefined) {
    value = title
    title = undefined
  }
  title = title ? `${title} -- ` : ""
  message = message ? `${message} -- ` : ""

  let output = truncate(stringify(value, null, 2), {
    length: 1000,
    omission: "...**only printing 1000 characters per log**",
  })
  return `${title}${message}${output}`
}

const logDebug = (...args: any[]) => {
  console.log(`DEBUG: ${logFormat(...args)}`)
}

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
    trigger: Observable<A>
  ) => { [key: string]: Observable<any> }
) => (trigger: Observable<A>) =>
  mapValues(fn(trigger), (obs, key) =>
    obs.pipe(
      tap((value) => logDebug(`${name}.${key}`, value)),
      share()
    )
  )

export const createTrigger = <A>(name: string, obs: Observable<A>) => {
  obs.subscribe((x) => {
    console.log("Trigger created for ", name)
    TriggerChannel.next({ name: `${name}`, payload: x } as Trigger)
  })
}

export const fromTrigger = (name: string): Observable<any> =>
  TriggerChannel.pipe(
    tap(x => console.log("TRIGGER CHANNEL", x)),
    // Filter out triggers by name. Note this allows for partial, case-insensitive string matching,
    // so filtering for "failure" will emit every time any trigger with "failure" in the name fires.
    filter((x: Trigger) => x.name.toLowerCase().includes(name.toLowerCase())),
    // Flatten the trigger so that it can be consumed by a subscriber without transforms
    map((x: Trigger) => ({ ...x.payload, name: x.name }))
  )