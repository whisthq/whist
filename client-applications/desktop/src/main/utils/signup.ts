import { post } from "@app/utils/api"
import { AsyncReturnType } from "@app/@types/state"

/* eslint-disable @typescript-eslint/naming-convention */

export const emailSignup = async (
  username: string,
  password: string,
  encrypted_config_token: string
) =>
  post({
    endpoint: "/account/register",
    body: {
      username,
      password,
      encrypted_config_token,
      name: "",
      feedback: "",
    },
  })

export type ResponseAuth = AsyncReturnType<typeof emailSignup>

export const emailSignupValid = (response: ResponseAuth) =>
  (response?.json?.access_token ?? "") !== ""

export const emailSignupError = (response: ResponseAuth) => {
  // A 400 bad response indicates that the user account exists,
  // we consider this a warning, not a failure.
  if (response?.status === 500) return false
  if (response?.status === 200) return false
  return true
}

export const emailSignupAccessToken = (response: ResponseAuth) =>
  response?.json?.access_token

export const emailSignupRefreshToken = (response: ResponseAuth) =>
  response?.json?.refresh_token
