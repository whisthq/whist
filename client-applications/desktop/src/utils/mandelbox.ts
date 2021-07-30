/**
 * Copyright Fractal Computers, Inc. 2021
 * @file mandelbox.ts
 * @brief This file contains utility functions interacting with the webserver to create mandelboxes.
 */
import { isEmpty } from "lodash"
import { get, post } from "@app/utils/api"
import { defaultAllowedRegions, AWSRegion } from "@app/@types/aws"
import { chooseRegion } from "@app/utils/region"
import { AsyncReturnType } from "@app/@types/state"
import { appEnvironment, FractalEnvironments } from "../../config/configs"
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

export const regionGet = async (subClaim: string, accessToken: string) => {
  const regions: Record<string, any> = await regionRequest(
    subClaim,
    accessToken
  )
  const allowedRegions = (regions?.json as AWSRegion[]) ?? []

  if (allowedRegions.length === 0) {
    return await chooseRegion(defaultAllowedRegions)
  } else {
    return await chooseRegion(regions.json)
  }
}

export const mandelboxCreate = async (
  subClaim: string,
  accessToken: string,
  region?: AWSRegion
) => {
  region = region ?? (await regionGet(subClaim, accessToken))
  const response = await mandelboxRequest(subClaim, accessToken, region)
  return response
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
const mandelboxRequest = async (
  username: string,
  accessToken: string,
  region: string
) =>
  post({
    endpoint: "/mandelbox/assign",
    accessToken,
    body: {
      username,
      region,
      client_commit_hash: isLocalEnv() ? "local_dev" : COMMIT_SHA,
    },
  })

const regionRequest = async (username: string, accessToken: string) =>
  get({
    endpoint: `/regions?username=${username}`,
    accessToken,
  })
