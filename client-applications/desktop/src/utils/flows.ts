import { Observable } from "rxjs"
import { filter, share, tap, map, mergeMap } from "rxjs/operators"
import { mapValues, truncate, values } from "lodash"
import stringify from "json-stringify-safe"
import { withMocking } from "@app/main/testing"

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

export const fork = <T, A extends { [key: string]: (result: T) => boolean }>(
  source: Observable<T>,
  filters: A
): { [P in keyof A]: Observable<T> } => {
  const shared = source.pipe(share())
  return mapValues(filters, (fn) => shared.pipe(filter(fn)))
}

export const flow =
  <T extends Observable<any>>(
    name: string,
    fn: (trigger: T) => { [key: string]: Observable<any> }
  ) =>
  (trigger: T) => {
    let channels = fn(trigger)

    return mapValues(withMocking(name, trigger, channels), (obs, key) =>
      obs.pipe(
        tap((value) => logDebug(`${name}.${key}`, value)),
        share()
      )
    )
  }
