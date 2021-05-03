import jwtDecode from "jwt-decode"
import { URL } from "url"
import { auth0Config } from "@app/config/environment"
import { store, persist } from "@app/utils/persist"
import fetch from "node-fetch"
import { randomBytes } from "crypto"

const { apiIdentifier, auth0Domain, clientId } = auth0Config

const redirectUri = "http://localhost/callback"

export function getAuthenticationURL() {
  return (
    "https://" +
    auth0Domain +
    "/authorize?" +
    "audience=" +
    apiIdentifier +
    "scope=openid profile offline_access email&" +
    "response_type=code&" +
    "client_id=" +
    clientId +
    "&" +
    "redirect_uri=" +
    redirectUri
  )
}

export async function refreshTokens(refreshToken: string) {
  const refreshOptions = {
    method: "POST",
    headers: { "content-type": "application/json" },
    body: JSON.stringify({
      grant_type: "refresh_token",
      client_id: clientId,
      refresh_token: refreshToken,
    }),
  }

  const response = await fetch(
    `https://${auth0Domain}/oauth/token`,
    refreshOptions
  )
  const data = await response.json()

  const accessToken: string = data.access_token
  if (accessToken !== "") {
    await persist({ accessToken })
  }

  const profile: Record<string, string> | undefined = jwtDecode(data.id_token)
  if (profile != null) {
    await persist({
      email: profile.email,
    })
  }
}

export async function loadTokens(callbackURL: string) {
  const url = new URL(callbackURL)
  const code = url.searchParams.get("code")

  const exchangeOptions = {
    grant_type: "authorization_code",
    client_id: clientId,
    code,
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
  const data = await response.json()

  const accessToken: string = data.access_token
  const refreshToken: string = data.refresh_token

  if (refreshToken !== "") {
    await persist({ refreshToken })
  }
  if (accessToken !== "") {
    await persist({ accessToken })
  }
  if (data.id_token !== "") {
    const profile: Record<string, string> = jwtDecode(data.id_token)
    await persist({
      email: profile.email,
    })
  }
}

export async function logout() {
  store.delete("refreshToken")
}

export function getLogOutUrl() {
  return `https://${auth0Domain}/v2/logout`
}

// Returns true if JWT is expired, false otherwise
export function checkJWTExpired(token: string) {
  const jwt: any = jwtDecode(token)
  return jwt.exp < Date.now() / 1000
}

export function generateRandomConfigToken() {
  const buffer = randomBytes(48)
  return buffer.toString("base64")
}

export async function storeConfigToken(configToken: string) {
  await persist({ userConfigToken: configToken })
}
