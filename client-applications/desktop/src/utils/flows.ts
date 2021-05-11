import { Observable } from "rxjs"
import { filter, share, tap } from "rxjs/operators"
import { mapValues, truncate } from "lodash"
import stringify from "json-stringify-safe"

import { FlowReturnType } from "@app/@types/state"

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
) => (childName: string, trigger: Observable<A>):FlowReturnType =>
  mapValues(fn(`${name}.${childName}`, trigger), (obs) =>
    obs.pipe(
      share()
    )
  )
