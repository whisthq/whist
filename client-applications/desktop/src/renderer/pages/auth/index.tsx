import React, { useState } from "react"
import { Route } from "react-router-dom"

import Login from "@app/renderer/pages/auth/login"
import Signup from "@app/renderer/pages/auth/signup"
import { useMainState } from "@app/utils/state"

const Auth = () => {
    /*
        Description:
            Router for auth-related pages (e.g. login and signup)
    */

    const [mainState, setMainState] = useMainState()
    const [email, setEmail] = useState("")
    const [password, setPassword] = useState("")
    const [confirmPassword, setConfirmPassword] = useState("")

    const onLogin = () => {
        setMainState({
            email,
            loginRequest: { email, password }})
    }

    const onSignup = () => {
        console.log("Signed up!")
    }

    return (
        <>
            <Route
                exact
                path="/"
                render={() => (
                    <Login
                        email={email}
                        password={password}
                        warning={mainState.loginWarning}
                        loading={mainState.loginLoading}
                        onLogin={onLogin}
                        onChangeEmail={setEmail}
                        onChangePassword={setPassword}
                    />
                )}
            />
            <Route
                path="/auth/login"
                render={() => (
                    <Login
                        email={email}
                        password={password}
                        warning={mainState.loginWarning}
                        loading={mainState.loginLoading}
                        onLogin={onLogin}
                        onChangeEmail={setEmail}
                        onChangePassword={setPassword}
                    />
                )}
            />
            <Route
                path="/auth/signup"
                render={() => <Signup onSignup={onSignup} />}
            />
        </>
    )
}

export default Auth
