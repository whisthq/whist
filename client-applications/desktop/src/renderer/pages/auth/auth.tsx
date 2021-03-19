import React from "react"
import { Route } from "react-router-dom"

import Login from "@app/renderer/pages/auth/pages/login/login"
import Signup from "@app/renderer/pages/auth/pages/signup/signup"

const Auth = () => {
    const onLogin = () => {
        console.log("Logged in!")
    }

    const onSignup = () => {
        console.log("Signed up!")
    }

    return (
        <>
            <Route
                exact
                path="/"
                render={() => <Signup onSignup={onSignup} />}
            />
            <Route path="/auth" render={() => <Login onLogin={onLogin} />} />
        </>
    )
}

export default Auth
