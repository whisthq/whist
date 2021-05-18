import { Observable } from "rxjs"
import { filter, share, tap } from "rxjs/operators"
import { mapValues, truncate } from "lodash"
import stringify from "json-stringify-safe"
import loggingMap from "../main/logging"
import { get, identity } from "lodash"

const logFormat = (...args: any[]) => {
  let [title, value] = args
  let [flowName, key] = title.split(".").slice(-2)
  let [message, transformFn] = get(loggingMap, [flowName, key], [null, null])
  if (message) {
    if (!transformFn) {
      transformFn = identity
    }
    let output = truncate(stringify(transformFn(value), null, 2), {
      length: 1000,
      omission: "...**only printing 1000 characters per log**",
    })
    return `${title} -- ${message} -- ${output}`
  }
}

const logDebug = (...args: any[]) => {
  const formatted = logFormat(...args)
  if (formatted) {
    console.log(`DEBUG: ${formatted}`)
  }
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
  mapValues(fn(`${name}.${childName}`, trigger), (obs, key) =>
    obs.pipe(
      tap((value) => logDebug(`${name}.${childName}.${key}`, value)),
      share()
    )
  )
