import { post } from "@app/utils/api"
import { AsyncReturnType } from "@app/@types/state"
import { createConfigToken, decryptConfigToken } from "@app/utils/crypto"

export const emailLogin = async (username: string, password: string) =>
  post({
    endpoint: "/account/login",
    body: { username, password },
  })

type ResponseAuth = AsyncReturnType<typeof emailLogin>

export const emailLoginValid = (response: ResponseAuth) =>
  (response?.json?.access_token ?? "") !== ""

export const emailLoginError = (response: ResponseAuth) =>
  response?.status !== 200

export const emailLoginAccessToken = (response: ResponseAuth) =>
  response.json?.access_token

export const emailLoginRefreshToken = (response: ResponseAuth) =>
  response.json?.refresh_token

export const emailLoginConfigToken = async (
  response: ResponseAuth,
  password: string
) =>
  (response?.json?.encrypted_config_token ?? "") !== ""
    ? decryptConfigToken(response?.json?.encrypted_config_token, password)
    : await createConfigToken()
