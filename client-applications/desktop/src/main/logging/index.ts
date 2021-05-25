import { get, mapValues, identity } from "lodash"
import { Observable } from "rxjs"
import { logFormat } from "@app/main/logging/utils"
import schema from "@app/main/logging/schema"

export const withLogging = <
  T extends Observable<any>,
  U extends { [key: string]: Observable<any> }
>(
  name: string,
  _trigger: T,
  channels: U
): { [P in keyof U]: Observable<any> } => {
  // Search the map of mockFns for the name of this flow
  // Return channels unchanged if not found
  const logFunctions = get(schema, name, {})

  return mapValues(channels, (obs, key) => {
    // Don't log at all if the key has a null value.
    if (get(logFunctions, key) === null) return obs

    // If the key is undefined/doesn't exist, log with identity fn
    const [message = "", fn = identity] = get(logFunctions, key, [])

    // We'll do this as a subscription for now, but really we should
    // "tap" this output. We'll want to switch this as we move the app towards
    // a more disciplined approach to subscriptions.
    obs.subscribe((value) => {
      console.log(logFormat(`${name}.${key}`, message, fn(value)))
    })

    return obs
  })
}
