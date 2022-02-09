import React from "react"
import { Auth0Provider, useAuth0 } from "@auth0/auth0-react"
import { config } from "@app/constants/app"

const AuthProvider = (props: { children: JSX.Element | JSX.Element[] }) => (
  <Auth0Provider
    domain={config.AUTH_DOMAIN_URL}
    clientId={config.AUTH_CLIENT_ID}
    redirectUri={config.CLIENT_CALLBACK_URL}
  >
    {props.children}
  </Auth0Provider>
)

const LogoutButton = (props: {
  logout: (args: { returnTo: string }) => void
}) => (
  <button
    onClick={() => {
      props.logout({ returnTo: window.location.origin })
    }}
  >
    Log out
  </button>
)

const LoginButton = (props: { login: () => void }) => (
  <button onClick={props.login}>Log in</button>
)

const IsLoggedIn = (props: {
  logout: (args: { returnTo: string }) => void
}) => <LogoutButton logout={props.logout} />

const NotLoggedIn = (props: { login: () => void }) => (
  <LoginButton login={props.login} />
)

const AuthRenderer = () => {
  const { isAuthenticated, logout, loginWithRedirect } = useAuth0()

  if (isAuthenticated) return <IsLoggedIn logout={logout} />
  return <NotLoggedIn login={loginWithRedirect} />
}

export default () => <AuthProvider children={<AuthRenderer />} />
