/**
 * Copyright Fractal Computers, Inc. 2021
 * @file auth.ts
 * @brief This file contains utility functions for authentication, mostly interacting with
 * the auth0 server.
 */

import jwtDecode from "jwt-decode"
import events from "events"
import { URL } from "url"
import { isEmpty, pickBy } from "lodash"

import { config } from "@app/config/environment"
import { randomBytes } from "crypto"
import { configPost } from "@fractal/core-ts"

const { apiIdentifier, auth0Domain, clientId } = config.auth0

// URI that Auth0 redirects authenticated users to
const redirectUri = "http://localhost/callback"

// Configures HTTP endpoint for Auth0 server
const auth0HttpConfig = {
  server: `https://${auth0Domain}`,
}

const post = configPost(auth0HttpConfig)

// Auth0 HTTP endpoint for handling login/signup
export const authenticationURL = [
  `https://${auth0Domain}/authorize`,
  `?audience=${apiIdentifier}`,
  // We need to request the admin scope here. If the user is enabled for the admin scope
  // (e.g. marked as a developer in Auth0), then the returned JWT token will be granted
  // the admin scope, thus bypassing Stripe checks and granting additional privileges in
  // the webserver. If the user is not enabled for the admin scope, then the JWT token
  // will be generated but will not have the admin scope, as documented by Auth0 in
  // https://auth0.com/docs/scopes#requested-scopes-versus-granted-scopes
  "&scope=openid profile offline_access email admin",
  "&response_type=code",
  `&client_id=${clientId}`,
  `&redirect_uri=${redirectUri}`,
].join("")

// Custom Event Emitter for Auth0 events
export const auth0Event = new events.EventEmitter()

const extractTokens = (response: Record<string, string>) => {
  /*
  Description:
    Helper function that takes an Auth0 response and extracts a {userEmail, accessToken, refreshToken} object
  Arguments:
    Response (Record): Auth0 response object
  Returns:
    {userEmail, accessToken, refreshToken}
  */
  try {
    const profile: Record<string, string> = jwtDecode(response.id_token ?? "")
    const { email } = profile

    return {
      userEmail: email,
      refreshToken: response.refresh_token,
      accessToken: response.access_token,
    }
  } catch {
    return {
      userEmail: null,
      refreshToken: null,
      accessToken: null,
    }
  }
}

export const generateRefreshedAuthInfo = async (refreshToken: string) => {
  /*
  Description:
    Given a valid Auth0 refresh token, generates a new {userEmail, accessToken, refreshToken} object
  Arguments:
    refreshToken (string): Refresh token
  Returns:
    {userEmail, accessToken, refreshToken}
  */
  const response = await post({
    endpoint: "/oauth/token",
    headers: { "content-type": "application/json" },
    body: {
      grant_type: "refresh_token",
      client_id: clientId,
      refresh_token: refreshToken,
    },
  })

  const data = response.json
  return extractTokens(data)
}

export const authInfo = async (callbackURL: string) => {
  /*
  Description:
    Given a callback URL, generates an {userEmail, accessToken, refreshToken} response
  Arguments:
    callbackURL (string): Callback URL
  Returns:
    {userEmail, accessToken, refreshToken}
  */

  const url = new URL(callbackURL)
  const code = url.searchParams.get("code")

  const body = {
    grant_type: "authorization_code",
    client_id: clientId,
    code,
    redirect_uri: redirectUri,
  }

  const response = await post({
    endpoint: "/oauth/token",
    headers: { "content-type": "application/json" },
    body,
  })
  const data = response.json
  return extractTokens(data)
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

export const authInfoValid = (authInfo: {
  userEmail: string
  accessToken: string
  refreshToken: string
}) => {
  return isEmpty(pickBy(authInfo, (x) => (x ?? "") === ""))
}

export const isExpired = (accessToken: string) => {
  // Extract the expiry in seconds since epoch
  const profile: { exp: number } = jwtDecode(accessToken)
  // Get current time in seconds since epoch
  const currentTime = Date.now() / 1000
  // Allow for ten seconds so we don't compare the access token to the current time right
  // before the expiry
  const secondsBuffer = 10
  return currentTime + secondsBuffer > profile.exp
}
