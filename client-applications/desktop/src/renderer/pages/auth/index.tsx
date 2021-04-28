import React, { useState } from "react"
import { Route } from "react-router-dom"

import Login from "@app/renderer/pages/auth/login"
import { useMainState } from "@app/utils/ipc"

const Auth = () => {
  /*
        Description:
            Router for auth-related pages (e.g. login and signup)
    */

  const [mainState, setMainState] = useMainState()
  const [email, setEmail] = useState("")
  const [password, setPassword] = useState("")

  const clearPassword = () => {
    setPassword("")
  }

  const onLogin = () => {
    setMainState({
      email,
      loginRequest: { email, password },
    })
  }

  return (
    <>
      <Route
        exact
        path="/login"
        render={() => (
          <Login
            email={email}
            password={password}
            warning={mainState?.loginWarning}
            loading={mainState?.loginLoading}
            onLogin={onLogin}
            onNavigate={clearPassword}
            onChangeEmail={setEmail}
            onChangePassword={setPassword}
          />
        )}
      />
    </>
  )
}

export default Auth
