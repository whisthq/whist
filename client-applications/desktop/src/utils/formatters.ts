import { ChildProcess } from "child_process"
import { omit, pick } from "lodash"

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

export const formatContainer = (req: object) =>
  omit(req, [
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

export const formatHostConfig = (req: object) =>
  pick(req, ["response._readableState"])

export const formatHostInfo = (req: object) =>
  omit(req, ["status", "statusText", "response"])
