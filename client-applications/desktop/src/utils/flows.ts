import { Observable } from "rxjs"
import { filter, share, tap, map, mergeMap } from "rxjs/operators"
import { mapValues, truncate, values } from "lodash"
import stringify from "json-stringify-safe"
import { mockState } from "@app/main/testing"

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
    childName: string,
    trigger: Observable<A>
  ) => { [key: string]: Observable<any> }
) => (childName: string, trigger: Observable<A>) =>
{
  const channels = fn(`${name}.${childName}`, trigger)
  if ( ) {     // if is testing
    return values(channels)
  }
  return mapValues(channels, (obs, key) => {
    console.log("FLOW NAME: ", name, "FLOW KEY ", key)
    
    return obs.pipe(
      // mergeMap((value) => mockState(value, name, key)),
      tap((value) => logDebug(`${name}.${key}`, value)),
      share()
    )
  })
}