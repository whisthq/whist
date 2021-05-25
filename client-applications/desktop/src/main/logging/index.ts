import host from "@app/main/logging/host"
import { tap } from "rxjs/operators"
import { get, mapValues, truncate, identity, isEmpty } from "lodash"
import { Observable } from "rxjs"
import stringify from "json-stringify-safe"

// export const transformLog = (name: string) => {
//   const [flowName, key] = name.split(".").slice(-2)
//   const [message = "", fn = identity] = get(loggingMap, [flowName, key], [])
//   return [message, fn]
// }

const logFormat = (title: string, message: string, value: any) => {
  value = truncate(stringify(value, null, 2), {
    length: 1000,
    omission: "...**only printing 1000 characters per log**",
  })

  if (isEmpty(message)) return `${title} -- ${value}`
  return `${title} -- ${message} -- ${value}`
}

// const logDebug = (title: string, value: any) => {
//   const [message, fn] = transformLog(title)
//   const formatted = logFormat(title, message, fn(value))
//   console.log(`DEBUG: ${formatted}`)
// }

type LoggingMap = Record<string, Record<string, any[]>>

const loggingMap: LoggingMap = {
  ...host,
}

export const withLogging = <
  T extends Observable<any>,
  U extends { [key: string]: Observable<any> }
>(
  /*
    Hijacks flow observables with mock observables if running on test mode
    Arguments:
      name (str): name of the flow, ex. loginFlow, signupFlow, authFlow
      trigger (obs): observable trigger to mock
      channels (obj): map of key-observable pairs to replace with mock schema
  */
  name: string,
  trigger: T,
  channels: U
): { [P in keyof U]: Observable<any> } => {
  // Search the map of mockFns for the name of this flow
  // Return channels unchanged if not found
  const logFunctions = get(loggingMap, name, {})

  return mapValues(channels, (obs, key) => {
    const [message = "", fn = identity] = get(logFunctions, key, [])

    obs.subscribe((value) => {
      console.log(logFormat(name, message, fn(value)))
    })

    return obs
  })
}
