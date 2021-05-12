import { pick } from "lodash"

import { get } from "@app/utils/api"
import { AsyncReturnType } from "@app/@types/state"

const taskStatus = async (taskID: string, accessToken: string) =>
  get({ endpoint: "/status/" + taskID, accessToken })

export const containerInfo = async (taskID: string, accessToken: string) =>
  await taskStatus(taskID, accessToken)

export const containerInfoError = (
  response: AsyncReturnType<typeof containerInfo>
) => (response?.json?.state ?? "") === ""

export const containerInfoSuccess = (
  response: AsyncReturnType<typeof containerInfo>
) => response?.json?.state === "SUCCESS"

export const containerInfoPending = (
  response: AsyncReturnType<typeof containerInfo>
) => response?.json?.state !== "SUCCESS" && response?.json?.state !== "FAILURE"

export const containerInfoPorts = (
  response: AsyncReturnType<typeof containerInfo>
) => pick(response?.json?.output, ["port_32262", "port_32263", "port_32273"])

export const containerInfoIP = (
  response: AsyncReturnType<typeof containerInfo>
) => response?.json?.output?.ip

export const containerInfoSecretKey = (
  response: AsyncReturnType<typeof containerInfo>
) => response?.json?.output?.secret_key
