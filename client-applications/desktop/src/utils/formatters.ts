/**
 * Copyright Fractal Computers, Inc. 2021
 * @file formatters.ts
 * @brief This file contains observable formatters for our debug logs
 */

import { ChildProcess } from "child_process"
import { omit, pick, truncate, fromPairs, toPairs, isFunction } from "lodash"
import { Observable } from "rxjs"
import { map } from "rxjs/operators"

/*
   Contains general fields of a typical JSON response that are usually
   unecessary, such as 'status' and 'response'
*/

const omitJson = ["status", "statusText", "response"]

export const formatObservable = (
  /*
    Description:
      General purpose formatter observable that applies a formatter function
      to the emitted output of a given observable

    Arguments:
      obs: An observable whos emissions are desired to be formatted
      formatter: a function that applies formatting to the output emitted from obs

    Returns:
      An observable that emits the formatted output from obs
  */
  obs: Observable<any>,
  formatter: (res: any) => void
) => obs.pipe(map((res) => formatter(res)))

export const formatChildProcess = (
  /*
    Description:
      formats debug output from protocol observables

    Arguments:
      process: Child process, in this case the process launching the protocol

    Returns:
      function omitting unecessary fields such as 'stdin' and 'stdio'
      using lodash's 'omit' function,
  */
  process: ChildProcess
) =>
  omit(process, [
    "stdin",
    "stdio",
    "stdout",
    "stderr",
    "_events",
    "_eventsCount",
    "_closesNeeded",
    "_closesGot",
    "_handle",
  ])

const pickMap = <
  T extends Record<string, any>,
  K extends { [P in keyof T]?: ((v: T[P]) => any) | K }
>(
  /*
    Description:
      Recursively flattens json objects into a single level of key-value pairs

    Arguments:
      obj: A Record of type String, any to be spreaded to a new object
      fmap: Map of functions to apply to key value pairs in obj

    Returns:
      JSON object from flattend obj and fmap
  */
  obj: T,
  fmap: K
): T => ({
  ...obj,
  ...fromPairs(
    toPairs(fmap).map(([key, value]) => [
      key,
      isFunction(value) ? value(obj[key]) : pickMap(obj[key], value),
    ])
  ),
})

export const formatLogin = (
  /*
    Description:
      Formats login json response using pickMap, applies truncate() to
      user hash tokens

    Arguments:
      res: JSON api response from login

    Returns:
      Function that formats given JSON object
  */
  res: any
) =>
  pickMap(omit(res, [...omitJson]), {
    json: {
      access_token: (token: string) => truncate(token, { length: 15 }),
      refresh_token: (token: string) => truncate(token, { length: 15 }),
      encrypted_config_token: (token: string) =>
        truncate(token, { length: 15 }),
    },
  })

export const formatMandelbox = (
  /*
    Description:
      Formats output from mandelbox observables. Only returns full output if state is
      something other than polling, aside from the usual unecessary JSON values in omitJson

    Args:
      res: JSON object emitted from mandelbox observable

    Returns:
      Function that formats given json object, for the polling state will omit
      all unecessary values, for everything else will only omit unecessary json values
      from omitJson
  */

  res: any
) => {
  if (res.state !== "POLLING") {
    return omit(res, [...omitJson, "json.output"])
  } else {
    return omit(res, [
      "statusText",
      "response",
      "status",
      "json.output.dpi",
      "json.output.ip",
      "json.output.location",
      "json.output.port_32262",
      "json.output.port_32263",
      "json.output.port_32273",
      "json.output.secret_key",
      "json.output.user_id",
      "json.output.last_updated_utc_unix_ms",
    ])
  }
}

// formats hostConfig obsrvables by picking only necessary fields
export const formatHostConfig = (res: object) => pick(res, ["json", "status"])

// formats hostInfo observables by omitting omitJson values
export const formatHostInfo = (res: object) => omit(res, [...omitJson])

// formats user login tokens by truncating
export const formatTokens = (token: string) => truncate(token, { length: 15 })

// formats tokens array from user login/sign up through truncation
export const formatTokensArray = (res: [string, string]) => {
  return [res[0], truncate(res[1], { length: 15 })]
}
