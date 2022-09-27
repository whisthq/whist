/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file auth.ts
 * @brief This file contains utility functions authentication.
 */

import jwtDecode from "jwt-decode"
import { Buffer } from "buffer"

import { post } from "@app/@core-ts/api"

import { config } from "@app/constants/app"

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

    const response = await post({
      url: `https://${<string>config.AUTH0_DOMAIN_URL}/oauth/token`,
      body: {
        grant_type: "authorization_code",
        client_id: config.AUTH0_CLIENT_ID,
        code,
        redirect_uri: config.AUTH0_REDIRECT_URL,
      },
    })

    return response
  } catch (err) {}
}

export const parseAuthInfo = (res: {
  status: number
  json: {
    id_token?: string
    access_token?: string
    refresh_token?: string
  }
}) => {
  /*
  Description:
    Helper function that takes an Auth0 response and extracts a {email, sub, accessToken} object
  Arguments:
    Response (Record): Auth0 response object
  Returns:
    {email, sub, accessToken, subscriptionStatus}
  */

  try {
    const accessToken = res?.json.access_token
    const decodedAccessToken = jwtDecode<any>(accessToken ?? "")
    const decodedIdToken = jwtDecode<any>(res?.json.id_token ?? "")
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
    return {
      userEmail,
      accessToken,
      subscriptionStatus,
      refreshToken: res?.json.refresh_token,
    }
  } catch (err) {
    return {
      error: {
        message: `Error while decoding JWT: ${err as string}`,
        data: res,
      },
    }
  }
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
    console.log(`Failed to decode access token: ${<string>err}`)
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
  return await post({
    url: `https://${<string>config.AUTH0_DOMAIN_URL}/oauth/token`,
    body: {
      grant_type: "refresh_token",
      client_id: config.AUTH0_CLIENT_ID,
      refresh_token: refreshToken,
      scope: "openid profile offline_access email",
    },
  })
}
