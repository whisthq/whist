import React, { useEffect } from "react"
import { connect } from "react-redux"
import { Redirect } from "react-router"

import Header from "shared/components/header"
import LoginView from "pages/auth/views/loginView"
import SignupView from "pages/auth/views/signupView"
import ForgotView from "pages/auth/views/forgotView"

import "styles/auth.css"

import history from "shared/utils/history"
//import { resetState } from "store/actions/shared"

const Auth = (props: {
    dispatch: any
    user: {
        user_id: string
        emailVerified: boolean
        canLogin: boolean
    }
    waitlistUser: {
        user_id: string
    }
    mode: any
    authFlow: any
    match: any
}) => {
    const { user, waitlistUser, match, mode } = props

    useEffect(() => {
        const firstParam = match.params.first
        if (firstParam !== "bypass" && !waitlistUser.user_id) {
            history.push("/")
        }
        if (firstParam === "bypass") {
            console.log("bypass login")
        }
    }, [match, waitlistUser.user_id])

    if (user.user_id && user.user_id !== "") {
        if (user.emailVerified) {
            return <Redirect to="/dashboard" />
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
    return {
        waitlistUser: state.WaitlistReducer.waitlistUser,
        mode: state.AuthReducer.authFlow.mode,
        user: state.AuthReducer.user,
        authFlow: state.AuthReducer.authFlow,
    }
}

export default connect(mapStateToProps)(Auth)
