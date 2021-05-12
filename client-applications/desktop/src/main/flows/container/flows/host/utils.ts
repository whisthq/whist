import { AsyncReturnType } from "@app/@types/state"
import { get, apiPut } from "@app/utils/api"
import { HostServicePort } from "@app/utils/constants"

// This file directly interacts with data returned from the webserver, which
// has keys labelled in Python's snake_case format. We want to be able to pass
// these objects through to functions that validate and parse them. We also
// want to make use of argument destruction in functions. We also don't want
// to obfuscate the server's data format by converting everything to camelCase.
// So we choose to just ignore the linter rule.
/* eslint-disable @typescript-eslint/naming-convention */

export const hostServiceInfo = async (username: string, accessToken: string) =>
  get({
    endpoint: `/host_service?username=${encodeURIComponent(username)}`,
    accessToken,
  })

export const hostServiceConfig = async (
  ip: string,
  host_port: number,
  client_app_auth_secret: string,
  user_id: string,
  config_encryption_token: string
) => {
  return (await apiPut(
    "/set_config_encryption_token",
    `https://${ip}:${HostServicePort}`,
    {
      user_id,
      client_app_auth_secret,
      host_port,
      config_encryption_token,
    },
    true
  )) as { status: number }
}

type hostServiceInfoResponse = AsyncReturnType<typeof hostServiceInfo>

type HostServiceConfigResponse = AsyncReturnType<typeof hostServiceConfig>

export const hostServiceInfoIP = (res: hostServiceInfoResponse) => res.json?.ip

export const hostServiceInfoPort = (res: hostServiceInfoResponse) =>
  res?.json?.port

export const hostServiceInfoSecret = (res: hostServiceInfoResponse) =>
  res?.json?.client_app_auth_secret

export const hostServiceInfoValid = (res: hostServiceInfoResponse) =>
  res?.status === 200 &&
  (hostServiceInfoIP(res) ?? "") !== "" &&
  (hostServiceInfoPort(res) ?? "") !== "" &&
  (hostServiceInfoSecret(res) ?? "") !== ""

export const hostServiceInfoPending = (res: hostServiceInfoResponse) =>
  res?.status === 200 && !hostServiceInfoValid(res)

export const hostServiceConfigValid = (res: HostServiceConfigResponse) =>
  res?.status === 200

export const hostServiceConfigError = (res: HostServiceConfigResponse) =>
  !hostServiceConfigValid(res)
