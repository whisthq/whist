/**
 * Copyright Fractal Computers, Inc. 2021
 * @file mandelbox.ts
 * @brief This file contains utility functions interacting with the webserver to create mandelboxes.
 */
import { isEmpty } from "lodash"
import { post } from "@app/utils/api"
import { AWSRegion, defaultAllowedRegions } from "@app/@types/aws"
import { sortRegionByProximity } from "@app/utils/region"
import { AsyncReturnType } from "@app/@types/state"
import { appEnvironment, FractalEnvironments } from "../../config/configs"
import { logBase } from "@app/utils/logging"
import config from "@app/config/environment"

const COMMIT_SHA = config.keys.COMMIT_SHA

const isLocalEnv = () => {
  const isLocal = appEnvironment === FractalEnvironments.LOCAL
  if (!isLocal && (isEmpty(COMMIT_SHA) || COMMIT_SHA === undefined)) {
    console.log("COMMIT_SHA is empty when appEnvironment is not LOCAL!")
    console.log("No COMMIT_SHA may create issues communicating with server.")
  }
  return isLocal
}

export const regionGet = async () => {
  const sortedRegions = await sortRegionByProximity(defaultAllowedRegions)
  return sortedRegions
}

export const mandelboxCreate = async (accessToken: string) => {
  const regions = await regionGet()
  
  logBase(`AWS regions are [${regions}]`, {})
  
  return await mandelboxRequest(accessToken, regions)
}

export const mandelboxCreateSuccess = (
  response: AsyncReturnType<typeof mandelboxCreate>
) => [200, 202].includes(response.status as number)

export const mandelboxCreateErrorNoAccess = (
  response: AsyncReturnType<typeof mandelboxCreate>
) => response.status === 402

export const mandelboxCreateErrorUnauthorized = (
  response: AsyncReturnType<typeof mandelboxCreate>
) => response.status === 422 || response.status === 401

export const mandelboxCreateErrorMaintenance = (
  response: AsyncReturnType<typeof mandelboxCreate>
) => response.status === 512

export const mandelboxCreateErrorInternal = (
  response: AsyncReturnType<typeof mandelboxCreate>
) =>
  (response?.json?.ID ?? "") === "" &&
  !mandelboxCreateErrorNoAccess(response) &&
  !mandelboxCreateErrorUnauthorized(response) &&
  !mandelboxCreateErrorMaintenance(response)

// Helper functions
const mandelboxRequest = async (accessToken: string, regions: AWSRegion[]) =>
  post({
    endpoint: "/mandelbox/assign",
    accessToken,
    body: {
      regions,
      client_commit_hash: isLocalEnv() ? "local_dev" : COMMIT_SHA,
    },
  })
