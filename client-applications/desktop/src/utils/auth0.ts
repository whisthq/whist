import jwtDecode from "jwt-decode"
import url from "url"
import { auth0Config } from '@app/utils/config'
import { store, persist } from '@app/utils/persist'
import fetch from 'node-fetch'

const { apiIdentifier, auth0Domain, clientId } = auth0Config

const redirectUri = "http://localhost/callback"

let accessToken: any  = null
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
    "audience=https://api.fractal.co/&" + 
    "scope=openid profile offline_access email&" +
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

  const refreshOptions = {
    method: "POST",
    headers: { "content-type": "application/json" },
    data: {
      grant_type: "refresh_token",
      client_id: clientId,
      refresh_token: refreshToken,
    }
  }

  const response = await fetch(`https://${auth0Domain}/oauth/token`, refreshOptions)
  const data = await response.json()

  accessToken = data.access_token
  if (accessToken) {
    await persist({accessToken})
  }

  profile = jwtDecode(data.id_token)
  if (profile) {
    await persist({
      email: profile.email
    })
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
    body: JSON.stringify(exchangeOptions),
  }
    const response = await fetch(`https://${auth0Domain}/oauth/token`, options)
    const data = await response.json();

    accessToken = data.access_token
    profile = jwtDecode(data.id_token)
    refreshToken = data.refresh_token

    if (refreshToken) {
      await persist({refreshToken})
    }
    if (accessToken) {
      await persist({accessToken})
    }
    if (profile) {
      await persist({
        email: profile.email
      })
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

// Returns true if JWT is expired, false otherwise
export function checkJWTExpired(token: string) {
  const jwt: any = jwtDecode(token)
  return jwt.exp < (Date.now() / 1000)
}
