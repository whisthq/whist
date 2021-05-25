import { get, mapValues, identity } from "lodash"
import { zip } from "rxjs"
import { consoleLog, amplitudeLog, fileLog } from "@app/main/logging/utils"
import schema from "@app/main/logging/schema"
import { FlowEffect } from "@app/@types/schema"

export const withLogging: FlowEffect = (context, name, _trigger, channels) => {
  // Search the map of mockFns for the name of this flow
  // Return channels unchanged if not found
  const logFunctions = get(schema, name, {})

  return mapValues(channels, (obs, key) => {
    // Don't log at all if the key has a null value.
    if (get(logFunctions, key) === null) return obs

    // If the key is undefined/doesn't exist, log with identity fn
    const [message = "", fn = identity] = get(logFunctions, key, [])

    const title = `${name}.${key}`

    // We'll do this as a subscription for now, but really we should
    // "tap" this output. We'll want to switch this as we move the app towards
    // a more disciplined approach to subscriptions.
    zip([context, obs]).subscribe(([_, value]) => {
      consoleLog(context.getValue(), title, message, fn(value))
      amplitudeLog(context.getValue(), title, message, value) //unformatted for amplitude
      fileLog(context.getValue(), title, message, fn(value))
    })

    return obs
  })
}
