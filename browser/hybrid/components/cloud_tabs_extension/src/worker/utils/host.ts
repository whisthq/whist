/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file host.ts
 * @brief This file contains utility functions for interacting with the host service.
 */

import { put } from "@app/@core-ts/api"

import { config } from "@app/constants/app"
import { AsyncReturnType } from "@app/@types/api"

// This file directly interacts with data returned from the scaling service, which
// has keys labelled in snake_case format. We want to be able to pass
// these objects through to functions that validate and parse them. We also
// want to make use of argument destruction in functions. We also don't want
// to obfuscate the server's data format by converting everything to camelCase.
// So we choose to just ignore the linter rule.
/* eslint-disable @typescript-eslint/naming-convention */
const hostSpinUp = async ({
  ip,
  config_encryption_token,
  is_new_config_encryption_token,
  jwt_access_token,
  mandelbox_id,
  json_data,
  importedData,
}: {
  ip: string
  config_encryption_token: string
  is_new_config_encryption_token: boolean
  jwt_access_token: string
  mandelbox_id: string
  json_data: string
  importedData:
    | {
        cookies?: string
        bookmarks?: string
        extensions?: string
        preferences?: string
        localStorage?: string
      }
    | undefined
}) => {
  return await put({
    body: {
      config_encryption_token,
      is_new_config_encryption_token: is_new_config_encryption_token ?? false,
      mandelbox_id,
      json_data,
      ip,
    },
    url: `${config.SCALING_SERVICE_URL as string}/json_transport`,
    accessToken: jwt_access_token,
  })
}

type HostSpinUpResponse = AsyncReturnType<typeof hostSpinUp>

const hostSpinUpSuccess = (res: HostSpinUpResponse) => {
  const result = res.json?.result
  return (
    (result?.port_32262 !== undefined &&
      result?.port_32263 !== undefined &&
      result?.port_32273 !== undefined &&
      result?.aes_key !== undefined &&
      true) ??
    false
  )
}

export { hostSpinUp, hostSpinUpSuccess }
