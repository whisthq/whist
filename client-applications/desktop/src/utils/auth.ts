import jwtDecode from "jwt-decode"
import { URL } from "url"
import { config } from "@app/config/environment"
import { persist } from "@app/utils/persist"
import { randomBytes } from "crypto"
import { configPost } from "@fractal/core-ts"

const { apiIdentifier, auth0Domain, clientId } = config.auth0

const redirectUri = "http://localhost/callback"

const auth0HttpConfig = {
  server: `https://${auth0Domain}`,
}
const post = configPost(auth0HttpConfig)

export const getAuthenticationURL = () => {
  return [
    `https://${auth0Domain}/authorize`,
    `?audience=${apiIdentifier}`,
    "&scope=openid profile offline_access email",
    "&response_type=code&",
    `client_id=${clientId}&`,
    `redirect_uri=${redirectUri}`,
  ].join("")
}

export const refreshTokens = async (refreshToken: string) => {
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

export const loadTokens = async (callbackURL: string) => {
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

export const extractTokens = (response: Record<string, string>) => {
  const profile: Record<string, string> = jwtDecode(response.id_token)
  const { sub, email } = profile
  return {
    sub,
    email,
    refreshToken: response.refresh_token,
    accessToken: response.access_token,
  }
}

// Returns true if JWT is expired, false otherwise
export const checkJWTExpired = (token: string) => {
  const jwt: any = jwtDecode(token)
  return jwt.exp < Date.now() / 1000
}

export const generateRandomConfigToken = () => {
  const buffer = randomBytes(48)
  return buffer.toString("base64")
}

export const storeConfigToken = async (configToken: string) => {
  await persist({ userConfigToken: configToken })
}
