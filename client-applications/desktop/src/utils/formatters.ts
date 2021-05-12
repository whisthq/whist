import { ChildProcess } from "child_process"
import { omit, pick, truncate, fromPairs, toPairs, isFunction } from "lodash"
import { Observable } from "@app/main/triggers/node_modules/rxjs"
import { map } from "rxjs/operators"

const omitJson = ["status", "statusText", "response"]

export const formatObservable = (
  obs: Observable<any>,
  formatter: (res: any) => void
) => obs.pipe(map((res) => formatter(res)))

export const formatChildProcess = (process: ChildProcess) =>
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

export const formatLogin = (res: any) =>
  pickMap(omit(res, [...omitJson]), {
    json: {
      access_token: (token: string) => truncate(token, { length: 15 }),
      refresh_token: (token: string) => truncate(token, { length: 15 }),
      encrypted_config_token: (token: string) =>
        truncate(token, { length: 15 }),
    },
  })

export const formatContainer = (res: any) => {
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
      "json.output.task_definition",
      "json.output.task_version",
      "json.output.user_id",
      "json.output.last_pinged",
    ])
  }
}

export const formatHostConfig = (res: object) => pick(res, ["json", "status"])

export const formatHostInfo = (res: object) => omit(res, [...omitJson])

export const formatTokens = (token: string) => truncate(token, { length: 15 })

export const formatTokensArray = (res: [string, string]) => {
  return [res[0], truncate(res[1], { length: 15 })]
}
