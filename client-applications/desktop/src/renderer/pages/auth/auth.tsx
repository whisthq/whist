import React from "react"
import { Route } from "react-router-dom"

import Login from "@app/renderer/pages/auth/pages/login/login"

const Auth = () => {
    const onLogin = () => {
        console.log("Logged in!")
    }

    const onSignup = () => {
        console.log("Signed up!")
    }

    return (
        <>
            <Route exact path="/" render={() => <Login onLogin={onLogin} />} />
            <Route path="/auth" render={() => <Login onLogin={onLogin} />} />
        </>
    )
}

export default Auth 