import { pick, omit, get } from "lodash"
import { LoggingFormat, LoggingSchema } from "@app/main/logging/utils"

const outputContainerPollingResponse: LoggingFormat = [
  "only json.msg/state",
  (val: any) => ({
    state: get(val, ["json", "state"]),
    msg: get(val, ["json", "output", "msg"]),
  }),
]

const outputContainerPollingSuccess: LoggingFormat = [
  "only select json keys",
  (val: any) =>
    omit(get(val, ["json", "output"]), [
      "cluster",
      "container_id",
      "secret_key",
      "ip",
      "last_pinged",
      "task_definition",
      "task_version",
      "user_id",
    ]),
]

const outputStatus: LoggingFormat = ["only status", (val: any) => val.status]

const outputJson: LoggingFormat = ["only json", (val: any) => val.json]

const schema: LoggingSchema = {
  containerPollingInner: {
    pending: outputContainerPollingResponse,
    success: outputContainerPollingResponse,
  },
  containerPollingRequest: {
    success: outputContainerPollingResponse,
    pending: outputContainerPollingResponse,
  },
  containerPollingFlow: {
    pending: outputContainerPollingResponse,
    success: outputContainerPollingSuccess,
  },
  protocolLaunchFlow: {
    success: [
      "only partial ChildProcess object",
      (val: any) => pick(val, ["pid", "killed", "spawnfile", "exitCode"]),
    ],
  },
  hostServiceInfoRequest: {
    success: outputJson,
    pending: outputJson,
  },
  hostPollingInner: {
    success: outputContainerPollingResponse,
    pending: outputJson,
  },
  hostServiceFlow: {
    success: outputStatus,
    pending: outputJson,
  },
  hostConfigFlow: {
    success: [
      "only json and status",
      (val: any) => pick(val, ["json", "status"]),
    ],
  },
  protocolCloseFlow: {
    success: [
      "only partial ChildProcess object",
      (val: any) => pick(val, ["killed", "exitCode"]),
    ],
  },
}

export default schema
