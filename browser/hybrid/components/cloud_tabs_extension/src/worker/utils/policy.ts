/**
 * Copyright (c) 2022 Whist Technologies, Inc.
 * @file policy.ts
 * @brief This file contains utility functions for downloading and applying policies.
 */

import { get } from "@app/@core-ts/api"

import { config } from "@app/constants/app"
import { AsyncReturnType } from "@app/@types/api"

const getPolicies = (accessToken: string) =>
  get({
    url: `${config.POLICY_SERVICE_URL as string}/api/policies`,
    accessToken,
  })

type GetPoliciesResponse = AsyncReturnType<typeof getPolicies>

const getPoliciesSuccess = (res: GetPoliciesResponse) => {
  if (res.json?.error !== "") {
    return false
  }
  return res.json?.policy !== undefined
}

const applyPolicies = (policies: any) => {
  ;(chrome as any).whist.updatePolicies(JSON.stringify(policies))
}

export { getPolicies, getPoliciesSuccess, GetPoliciesResponse, applyPolicies }
