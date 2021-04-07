import { AsyncReturnType } from '@app/utils/types'
import { get } from '@app/utils/api'
import { apiPut } from '@app/utils/misc'
import { HostServicePort } from '@app/utils/constants'

export const hostServiceInfo = async (username: string, accessToken: string) =>
  get({
    endpoint: `/host_service?username=${username}`,
    accessToken
  })

export const hostServiceConfig = async (
  ip: string,
  host_port: number,
  client_app_auth_secret: string,
  user_id: string,
  config_encryption_token: string
) => {
  return (await apiPut(
    '/set_config_encryption_token',
        `https://${ip}:${HostServicePort}`,
        {
          user_id,
          client_app_auth_secret,
          host_port,
          config_encryption_token
        },
        true
  )) as { status: number }
}

type hostServiceInfoResponse = AsyncReturnType<typeof hostServiceInfo>

type HostServiceConfigResponse = AsyncReturnType<typeof hostServiceConfig>

export const hostServiceInfoIP = (res: hostServiceInfoResponse) => res.json?.ip

export const hostServiceInfoPort = (res: hostServiceInfoResponse) =>
  res.json?.port

export const hostServiceInfoSecret = (res: hostServiceInfoResponse) =>
  res.json?.client_app_auth_secret

export const hostServiceInfoValid = (res: hostServiceInfoResponse) =>
  !!(res.status === 200 &&
    hostServiceInfoIP(res) &&
    hostServiceInfoPort(res) &&
    hostServiceInfoSecret(res))

export const hostServiceInfoPending = (res: hostServiceInfoResponse) =>
  res.status === 200 && !hostServiceInfoValid(res)

export const hostServiceConfigValid = (res: HostServiceConfigResponse) =>
  res.status === 200

export const hostServiceConfigError = (_: any) => !hostServiceInfoValid
