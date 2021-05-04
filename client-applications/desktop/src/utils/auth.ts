import jwtDecode from "jwt-decode"
import { URL } from "url"
import { auth0Config } from "@app/config/environment"
import { store, persist } from "@app/utils/persist"
import { randomBytes } from "crypto"
import { configPost } from "@fractal/core-ts"

const { apiIdentifier, auth0Domain, clientId } = auth0Config

const redirectUri = "http://localhost/callback"

const auth0HttpConfig = {
  server: `https://${auth0Domain}`,
}

const post = configPost(auth0HttpConfig)

export const getAuthenticationURL = () => {
  return `https://${auth0Domain}/authorize?audience=${apiIdentifier}&scope=openid profile offline_access email&response_type=code&client_id=${clientId}&redirect_uri=${redirectUri}`
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

export const logout = () => {
  store.delete("refreshToken")
}

export const getLogOutUrl = () => {
  return `https://${auth0Domain}/v2/logout`
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
