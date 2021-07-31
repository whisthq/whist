import jwtDecode from "jwt-decode"
import { randomBytes } from "crypto"
import { config } from "../config"
import { configPost } from "../"
import { authCallbackURL, refreshToken, accessToken } from "../types/data"

const post = configPost({ server: `https://${config.AUTH_DOMAIN_URL}` })

export const authInfoCallbackRequest = async ({
  authCallbackURL,
}: authCallbackURL) => {
  /*
  Description:
    Given a callback URL, generates an {email, sub, accessToken, refreshToken} response
  Arguments:
    callbackURL (string): Callback URL
  Returns:
    {email, sub, accessToken, refreshToken}
  */

  const url = new URL(authCallbackURL)
  const code = url.searchParams.get("code")

  return await post({
    endpoint: "/oauth/token",
    headers: { "content-type": "application/json" },
    body: {
      grant_type: "authorization_code",
      client_id: config.AUTH_CLIENT_ID,
      code,
      redirect_uri: config.CLIENT_CALLBACK_URL,
    },
  })
}

export const authInfoRefreshRequest = async ({
  refreshToken,
}: refreshToken) => {
  /*
  Description:
    Given a valid Auth0 refresh token, generates a new {email, sub, accessToken, refreshToken} object
  Arguments:
    refreshToken (string): Refresh token
  Returns:
    {email, sub, accessToken, refreshToken}
  */

  return await post({
    endpoint: "/oauth/token",
    headers: { "content-type": "application/json" },
    body: {
      grant_type: "refresh_token",
      client_id: config.AUTH_CLIENT_ID,
      refresh_token: refreshToken,
    },
  })
}

export const authPortalURL = () =>
  [
    `https://${config.AUTH_DOMAIN_URL}/authorize`,
    `?audience=${config.AUTH_API_IDENTIFIER}`,
    // We need to request the admin scope here. If the user is enabled for the admin scope
    // (e.g. marked as a developer in Auth0), then the returned JWT token will be granted
    // the admin scope, thus bypassing Stripe checks and granting additional privileges in
    // the webserver. If the user is not enabled for the admin scope, then the JWT token
    // will be generated but will not have the admin scope, as documented by Auth0 in
    // https://auth0.com/docs/scopes#requested-scopes-versus-granted-scopes
    "&scope=openid profile offline_access email admin",
    "&response_type=code",
    `&client_id=${config.AUTH_CLIENT_ID}`,
    `&redirect_uri=${config.CLIENT_CALLBACK_URL}`,
  ].join("")

export const authInfoParse = (res: any) => {
  /*
  Description:
    Helper function that takes an Auth0 response and extracts a {email, sub, accessToken, refreshToken} object
  Arguments:
    Response (Record): Auth0 response object
  Returns:
    {email, sub, accessToken, refreshToken}
  */

  const decoded = jwtDecode(res?.json?.id_token ?? "") as any
  const jwtIdentity = decoded?.sub
  const userEmail = decoded?.email
  const accessToken = res?.json?.access_token
  const refreshToken = res?.json?.refresh_token
  if (typeof jwtIdentity !== "string")
    return {
      error: {
        message: "Decoded JWT does not have property .sub",
        data: res,
      },
    }
  if (typeof userEmail !== "string")
    return {
      error: {
        message: "Decoded JWT does not have property .email",
        data: res,
      },
    }
  if (typeof accessToken !== "string")
    return {
      error: {
        message: "Response does not have .json.access_token",
        data: res,
      },
    }
  if (typeof refreshToken !== "string")
    return {
      error: {
        message: "Response does not have .json.refresh_token",
        data: res,
      },
    }
  return { jwtIdentity, userEmail, accessToken, refreshToken }
}

export const generateRandomConfigToken = () => {
  /*
  Description:
    Generates a config token for app config security
  Returns:
    string: Config token
  */

  const buffer = randomBytes(48)
  return buffer.toString("base64")
}

export const isTokenExpired = ({ accessToken }: accessToken) => {
  // Extract the expiry in seconds since epoch
  const profile: { exp: number } = jwtDecode(accessToken)
  // Get current time in seconds since epoch
  const currentTime = Date.now() / 1000
  // Allow for ten seconds so we don't compare the access token to the current time right
  // before the expiry
  const secondsBuffer = 10
  return currentTime + secondsBuffer > profile.exp
}
