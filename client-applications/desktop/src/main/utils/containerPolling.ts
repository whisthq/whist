import { pick } from "lodash"

import { get } from "@app/utils/api"
import { AsyncReturnType } from "@app/@types/state"

const taskStatus = async (taskID: string, accessToken: string) =>
  get({ endpoint: "/status/" + taskID, accessToken })

export const containerPolling = async (taskID: string, accessToken: string) =>
  await taskStatus(taskID, accessToken)

export const containerPollingError = (
  response: AsyncReturnType<typeof containerPolling>
) => (response?.json?.state ?? "") === ""

export const containerPollingSuccess = (
  response: AsyncReturnType<typeof containerPolling>
) => response?.json?.state === "SUCCESS"

export const containerPollingPending = (
  response: AsyncReturnType<typeof containerPolling>
) => response?.json?.state !== "SUCCESS" && response?.json?.state !== "FAILURE"

export const containerPollingPorts = (
  response: AsyncReturnType<typeof containerPolling>
) => pick(response?.json?.output, ["port_32262", "port_32263", "port_32273"])

export const containerPollingIP = (
  response: AsyncReturnType<typeof containerPolling>
) => response?.json?.output?.ip

export const containerPollingSecretKey = (
  response: AsyncReturnType<typeof containerPolling>
) => response?.json?.output?.secret_key
