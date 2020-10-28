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
        emailVerified: boolean
    }
    waitlistUser: {
        user_id: string
    }
    mode: any
}) => {
    const { user, waitlistUser, mode } = props

    if (!waitlistUser.user_id) {
        return <Redirect to="/" />
    }

    if (user.user_id && user.user_id !== "") {
        if (user.emailVerified) {
            return <Redirect to="/" />
        } else {
            return <Redirect to="/verify" />
        }
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
    } else if (mode === "Forgot") {
        return (
            <div className="fractalContainer">
                <Header color="black" />
                <ForgotView />
            </div>
        )
    } else {
        // should never happen
        return <div />
    }
}

function mapStateToProps(state: {
    AuthReducer: { authFlow: any; user: any }
    WaitlistReducer: { waitlistUser: any }
}) {
    console.log(state)
    return {
        waitlistUser: state.WaitlistReducer.waitlistUser,
        mode: state.AuthReducer.authFlow.mode,
        user: state.AuthReducer.user,
    }
}

export default connect(mapStateToProps)(Auth)
