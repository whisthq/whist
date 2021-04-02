import React, { useState, useEffect } from "react"
import { Route, useHistory } from "react-router-dom"

import Login from "@app/renderer/pages/auth/login"
import Signup from "@app/renderer/pages/auth/signup"
import { useMainState } from "@app/utils/state"

const Auth = () => {
    /*
        Description:
            Router for auth-related pages (e.g. login and signup)
    */

    const [mainState, setMainState] = useMainState()
    const [email, setEmail] = useState("fakeemail@gmail.com")
    const [password, setPassword] = useState("Password1234")
    const [confirmPassword, setConfirmPassword] = useState("Password1234")


    const clearPassword = () => {
        setPassword("")
        setConfirmPassword("")
    }

    const onLogin = () => {
        setMainState({
            email,
            loginRequest: { email, password }})
    }

    const onSignup = () => {
        setMainState({
            email,
            signupRequest: { email, password }})
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
                        warning={mainState.loginWarning}
                        loading={mainState.loginLoading}
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
                        warning={mainState.signupWarning}
                        loading={mainState.signupLoading}
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
