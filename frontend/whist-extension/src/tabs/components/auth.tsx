import React from "react"

import { openGoogleAuth } from "@app/tabs/utils/auth"

const LoginButton = (props: { login: () => void }) => (
  <button onClick={props.login}>Log in</button>
)

const LoginTitle = () => (
  <div className="bg-gray-300 text-2xl">Welcome to Whist</div>
)

const LoginSubtitle = () => (
  <div className="bg-gray-500 text-md">
    To unlock cloud tabs, please sign in or create an account
  </div>
)

const LoginComponent = () => {
  return (
    <div className="w-screen h-screen bg-gray-900">
      <LoginTitle />
      <LoginSubtitle />
      <LoginButton login={openGoogleAuth} />
    </div>
  )
}

export default LoginComponent
