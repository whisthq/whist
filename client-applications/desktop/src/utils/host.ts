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

const imageEnv = () => {
  switch (appEnvironment) {
    case FractalEnvironments.LOCAL:
      return "dev"
    case FractalEnvironments.DEVELOPMENT:
      return "dev"
    case FractalEnvironments.STAGING:
      return "staging"
    case FractalEnvironments.PRODUCTION:
      return "prod"
    default:
      return "dev"
  }
}

export const hostSpinUp = async ({
  ip,
  dpi,
  user_id,
  config_encryption_token,
  jwt_access_token,
  mandelbox_id,
}: {
  ip: string
  dpi: number
  user_id: string
  config_encryption_token: string
  jwt_access_token: string
  mandelbox_id: string
}) =>
  (await apiPut(
    "/spin_up_mandelbox",
    `https://${ip}:${HostServicePort}`,
    {
      app_image: `fractal/${imageEnv()}/browsers/chrome:current-build`,
      dpi,
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
    (result?.port_32262 &&
      result.port_32263 &&
      result?.port_32273 &&
      result?.aes_key &&
      true) ||
    false
  )
}

export const hostSpinUpError = (res: HostSpinUpResponse) =>
  !hostSpinUpValid(res)
