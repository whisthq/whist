import React, { useState } from "react"
import { Route } from "react-router-dom"

import Login from "@app/renderer/pages/auth/login"
import Signup from "@app/renderer/pages/auth/signup"
import { useMainState } from "@app/utils/ipc"
import { RendererAction } from "@app/@types/actions"

const Auth = () => {
  /*
        Description:
            Router for auth-related pages (e.g. login and signup)
    */

  const [mainState, setMainState] = useMainState()
  const [email, setEmail] = useState("neil@fractal.co")
  const [password, setPassword] = useState("Password1234")
  const [confirmPassword, setConfirmPassword] = useState("Password1234")

  console.log("STATE", mainState)

  const clearPassword = () => {
    setPassword("")
    setConfirmPassword("")
  }

  const onLogin = () => setMainState({login: { email, password },})

  const onSignup = () => setMainState({signup: { email, password },})

  return (
    <>
      <Route
        exact
        path="/login"
        render={() => (
          <Login
            email={email}
            password={password}
            warning={mainState?.login?.warning?.statusText}
            loading={mainState?.login?.loading}
            onLogin={onLogin}
            onNavigate={clearPassword}
            onChangeEmail={setEmail}
            onChangePassword={setPassword}
          />
        )}
      />
      <Route
        exact
        path="/"
        render={() => (
          <Signup
            email={email}
            password={password}
            confirmPassword={confirmPassword}
            warning={mainState?.signup?.warning?.statusText}
            loading={mainState?.signup?.loading}
            onSignup={onSignup}
            onNavigate={clearPassword}
            onChangeEmail={setEmail}
            onChangePassword={setPassword}
            onChangeConfirmPassword={setConfirmPassword}
          />
        )}
      />
    </>
  )
}

export default Auth
