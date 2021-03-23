import React from "react"
import { Route } from "react-router-dom"

import Login from "@app/renderer/pages/auth/login"
import Signup from "@app/renderer/pages/auth/signup"
import { useMainState } from "@app/utils/state"

const Auth = () => {
    /*
        Description:
            Router for auth-related pages (e.g. login and signup)
    */

    const [_state, setState] = useMainState()

    const onLogin = (json: any) => {
        console.log(json)
        setState({ accessToken: json.access_token || "",
                   refreshToken: json.refresh_token || "",
                   email: json.email || ""
        })
    }

    const onSignup = () => {
        console.log("Signed up!")
    }

    return (
        <>
            <Route exact path="/" render={() => <Login onLogin={onLogin} />} />
            <Route
                path="/auth/login"
                render={() => <Login onLogin={onLogin} />}
            />
            <Route
                path="/auth/signup"
                render={() => <Signup onSignup={onSignup} />}
            />
        </>
    )
}

export default Auth
