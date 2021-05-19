import { Observable } from "rxjs"
import { filter, share, tap } from "rxjs/operators"
import { mapValues, truncate } from "lodash"
import stringify from "json-stringify-safe"

import { transformLog } from "@app/utils/logging"

const logFormat = (title: string, message: string, value: any) => {
  value = truncate(stringify(value, null, 2), {
    length: 1000,
    omission: "...**only printing 1000 characters per log**",
  })
  return `${title} -- ${message} -- ${value}`
}

const logDebug = (title: string, value: any) => {
  const [message, fn] = transformLog(title)
  const formatted = logFormat(title, message, fn(value))
  console.log(`DEBUG: ${formatted}`)
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
