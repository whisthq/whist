import jwtDecode from "jwt-decode"

import { config } from "@app/constants/app"

const redirectURL = chrome.identity.getRedirectURL("auth0")

export const authPortalURL = () => {
  return [
    `https://${config.AUTH_DOMAIN_URL}/authorize`,
    `?audience=${config.AUTH_API_IDENTIFIER}`,
    // We need to request the admin scope here. If the user is enabled for the admin scope
    // (e.g. marked as a developer in Auth0), then the returned JWT token will be granted
    // the admin scope, thus bypassing Stripe checks and granting additional privileges in
    // the webserver. If the user is not enabled for the admin scope, then the JWT token
    // will be generated but will not have the admin scope, as documented by Auth0 in
    // https://auth0.com/docs/scopes#requested-scopes-versus-granted-scopes
    `&client_id=${config.AUTH_CLIENT_ID}`,
    `&redirect_uri=${redirectURL}`,
    `&connection=google-oauth2`,
    "&scope=openid profile offline_access email admin",
    "&response_type=code",
  ].join("")
}

export const authInfoCallbackRequest = async (
  callbackURL: string | undefined
) => {
  /*
  Description:
    Given a callback URL, generates an { email, accessToken, refreshToken } response
  Arguments:
    callbackURL (string): Callback URL
  Returns:
    {email, accessToken, refreshToken}
  */

  if (callbackURL === undefined) return

  try {
    const url = new URL(callbackURL)
    const code = url.searchParams.get("code")

    const response = await fetch(
      `https://${config.AUTH_DOMAIN_URL}/oauth/token`,
      {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          grant_type: "authorization_code",
          client_id: config.AUTH_CLIENT_ID,
          code,
          redirect_uri: redirectURL,
        }),
      }
    )

    return await response.json()
  } catch (err) {
    return
  }
}

export const parseAuthInfo = (res: {
  id_token?: string
  access_token?: string
}) => {
  /*
  Description:
    Helper function that takes an Auth0 response and extracts a {email, sub, accessToken, refreshToken} object
  Arguments:
    Response (Record): Auth0 response object
  Returns:
    {email, sub, accessToken, refreshToken, subscriptionStatus}
  */

  try {
    const accessToken = res?.access_token
    const decodedAccessToken = jwtDecode(accessToken ?? "") as any
    const decodedIdToken = jwtDecode(res?.id_token ?? "") as any
    const userEmail = decodedIdToken?.email
    const subscriptionStatus =
      decodedAccessToken["https://api.fractal.co/subscription_status"]

    if (typeof accessToken !== "string")
      return {
        error: {
          message: "Response does not have .json.access_token",
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
    return { userEmail, accessToken, subscriptionStatus }
  } catch (err) {
    return {
      error: {
        message: `Error while decoding JWT: ${err as string}`,
        data: res,
      },
    }
  }
}

export const generateRandomConfigToken = () => {
  /*
  Description:
    Generates a config token for app config security
  Returns:
    string: Config token
  */
  const crypto = new Crypto()
  const buffer = Buffer.from(crypto.getRandomValues(new Uint32Array(48)))
  return buffer.toString("base64")
}

export const isTokenExpired = (accessToken: string) => {
  // Extract the expiry in seconds since epoch
  try {
    const profile: { exp: number } = jwtDecode(accessToken)
    // Get current time in seconds since epoch
    const currentTime = Date.now() / 1000
    // Allow for ten seconds so we don't compare the access token to the current time right
    // before the expiry
    const secondsBuffer = 10
    return currentTime + secondsBuffer > profile.exp
  } catch (err) {
    console.log(`Failed to decode access token: ${err}`)
    return true
  }
}

export const requestAuthInfoRefresh = async (refreshToken: string) => {
  /*
  Description:
    Given a valid Auth0 refresh token, generates a new {email, accessToken, refreshToken} object
  Arguments:
    refreshToken (string): Refresh token
  Returns:
    {email, accessToken, refreshToken}
  */
  return await fetch(`https://${config.AUTH_DOMAIN_URL}/oauth/token`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({
      grant_type: "refresh_token",
      client_id: config.AUTH_CLIENT_ID,
      refresh_token: refreshToken,
    }),
  })
}
