/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file mandelbox.ts
 * @brief This file contains utility functions interacting with the scaling service to create mandelboxes.
 */
import { post } from "@app/@core-ts/api"

import { AWSRegion } from "@app/constants/location"
import { AsyncReturnType } from "@app/@types/api"
import { config, sessionID } from "@app/constants/app"

const COMMIT_SHA = config.COMMIT_SHA

export const mandelboxCreateSuccess = (
  response: AsyncReturnType<typeof mandelboxRequest>
) => [200, 202].includes(response.status as number)

export const mandelboxCreateErrorNoAccess = (
  response: AsyncReturnType<typeof mandelboxRequest>
) => response.status === 402

export const mandelboxCreateErrorUnauthorized = (
  response: AsyncReturnType<typeof mandelboxRequest>
) => response.status === 422 || response.status === 401

export const mandelboxCreateErrorMaintenance = (
  response: AsyncReturnType<typeof mandelboxRequest>
) => response.status === 512

export const mandelboxCreateErrorUnavailable = (
  response: AsyncReturnType<typeof mandelboxRequest>
) => response.status === 503 && response.json.error === "NO_INSTANCE_AVAILABLE"

export const mandelboxCreateErrorCommitHash = (
  response: AsyncReturnType<typeof mandelboxRequest>
) => response.status === 503 && response.json.error === "COMMIT_HASH_MISMATCH"

// Helper functions
export const mandelboxRequest = async (
  accessToken: string,
  regions: AWSRegion[],
  userEmail: string,
  version: string
) =>
  await post({
    body: {
      regions,
      client_commit_hash: COMMIT_SHA,
      session_id: sessionID,
      user_email: userEmail,
      version: version,
    },
    url: `${config.SCALING_SERVICE_URL}/mandelbox/assign`,
    accessToken,
  })
