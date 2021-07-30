/**
 * Copyright Fractal Computers, Inc. 2021
 * @file host.ts
 * @brief This file contains utility functions for interacting with the host service.
 */

import { AsyncReturnType } from "@app/@types/state"
import { apiPut } from "@app/utils/api"
import { HostServicePort } from "@app/utils/constants"
import { appEnvironment, FractalEnvironments } from "../../config/configs"

// This file directly interacts with data returned from the webserver, which
// has keys labelled in Python's snake_case format. We want to be able to pass
// these objects through to functions that validate and parse them. We also
// want to make use of argument destruction in functions. We also don't want
// to obfuscate the server's data format by converting everything to camelCase.
// So we choose to just ignore the linter rule.
/* eslint-disable @typescript-eslint/naming-convention */

export const hostSpinUp = async ({
  ip,
  user_id,
  config_encryption_token,
  jwt_access_token,
  mandelbox_id,
}: {
  ip: string
  user_id: string
  config_encryption_token: string
  jwt_access_token: string
  mandelbox_id: string
}) =>
  (await apiPut(
    "/spin_up_mandelbox",
    `https://${ip}:${HostServicePort}`,
    {
      app_name: `browsers/chrome`,
      user_id,
      config_encryption_token,
      jwt_access_token,
      mandelbox_id,
    },
    true
  )) as {
    status: number
    json?: {
      result?: {
        port_32262: number
        port_32263: number
        port_32273: number
        aes_key: string
      }
    }
  }

export type HostSpinUpResponse = AsyncReturnType<typeof hostSpinUp>

export const hostSpinUpValid = (res: HostSpinUpResponse) => {
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

export const hostSpinUpError = (res: HostSpinUpResponse) =>
  !hostSpinUpValid(res)
