import React from "react"
import { connect } from "react-redux"
import { Redirect } from "react-router"

import Header from "shared/components/header"
import LoginView from "pages/auth/views/loginView"
import SignupView from "pages/auth/views/signupView"

import "styles/auth.css"
import ForgotView from "./views/forgotView"

const Auth = (props: {
    dispatch: any
    user: {
        user_id: string
    }
    mode: any
}) => {
    const { user, mode } = props

    if (user.user_id && user.user_id !== "") {
        return <Redirect to="/" />
    }

    if (mode === "Log in") {
        return (
            <div className="fractalContainer">
                <Header color="black" />
                <LoginView />
            </div>
        )
    } else if (mode === "Sign up") {
        return (
            <div className="fractalContainer">
                <Header color="black" />

                <SignupView />
            </div>
        )
    } else if (mode === "Forgot password") {
        return (
            <div>
                <ForgotView />
            </div>
        )
    } else {
        // should never happen
        return <div />
    }
}

function mapStateToProps(state: { AuthReducer: { user: any; authFlow: any } }) {
    console.log(state.AuthReducer.authFlow)
    return {
        user: state.AuthReducer.user,
        mode: state.AuthReducer.authFlow.mode,
    }
}

export default connect(mapStateToProps)(Auth)
