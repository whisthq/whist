import jwtDecode from "jwt-decode"
import url from "url"
import { auth0Config } from '@app/utils/config'
import { store, persist } from '@app/utils/persist'

const { apiIdentifier, auth0Domain, clientId } = auth0Config

const redirectUri = "http://localhost/callback"

let accessToken: string | null = null
let profile: any = null
let refreshToken = null

export function getAccessToken() {
  return accessToken
}

export function getProfile() {
  return profile
}

export function getAuthenticationURL() {
  return (
    "https://" +
    auth0Domain +
    "/authorize?" +
    "scope=openid profile offline_access&" +
    "response_type=code&" +
    "client_id=" +
    clientId +
    "&" +
    "redirect_uri=" +
    redirectUri
  )
}

export async function refreshTokens() {
  const refreshToken = store.get("refreshToken")

  if (refreshToken) {
    const refreshOptions = {
      method: "POST",
      headers: { "content-type": "application/json" },
      data: {
        grant_type: "refresh_token",
        client_id: clientId,
        refresh_token: refreshToken,
      },
    }

    try {
      const response = await fetch(`https://${auth0Domain}/oauth/token`, refreshOptions)
      const data = await response.json()

      accessToken = data.access_token
      profile = jwtDecode(data.id_token)
    } catch (error) {
      await logout()

      throw error
    }
  } else {
    throw new Error("No available refresh token.")
  }
}

export async function loadTokens(callbackURL: string) {
  const urlParts = url.parse(callbackURL, true)
  const query = urlParts.query

  const exchangeOptions = {
    grant_type: "authorization_code",
    client_id: clientId,
    code: query.code,
    redirect_uri: redirectUri,
  }

  const options = {
    method: "POST",
    headers: {
      "content-type": "application/json",
    },
    data: JSON.stringify(exchangeOptions),
  }

  try {
    const response = await fetch(`https://${auth0Domain}/oauth/token`, options)
    const data = await response.json();

    accessToken = data.access_token
    profile = jwtDecode(data.id_token)
    refreshToken = data.refresh_token

    if (refreshToken) {
      await persist({refreshToken})
    }
  } catch (error) {
    await logout()

    throw error
  }
}

export async function logout() {
  store.delete('refreshToken')
  accessToken = null
  profile = null
  refreshToken = null
}

export function getLogOutUrl() {
  return `https://${auth0Domain}/v2/logout`
}
