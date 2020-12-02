import React, { useEffect } from "react"
import { connect } from "react-redux"
import { Redirect } from "react-router"

import Header from "shared/components/header"
import LoginView from "pages/auth/views/loginView"
import SignupView from "pages/auth/views/signupView"
import ForgotView from "pages/auth/views/forgotView"

import "styles/auth.css"

import history from "shared/utils/history"
import { updateUser } from "store/actions/auth/pure"

const Auth = (props: {
    dispatch: any
    user: {
        userID: string
        emailVerified: boolean
        canLogin: boolean
    }
    waitlistUser: {
        userID: string
    }
    mode: any
    authFlow: any
    match: any
}) => {
    const { user, waitlistUser, match, mode, dispatch } = props

    useEffect(() => {
        const firstParam = match.params.first
        const secondParam = match.params.second

        if (firstParam !== "bypass" && !waitlistUser.userID) {
            history.push("/")
        }

        if (secondParam && secondParam !== "") {
            dispatch(updateUser({ waitlistToken: secondParam }))
        }
    }, [match, waitlistUser.userID, dispatch])

    if (user.userID && user.userID !== "") {
        if (user.emailVerified) {
            return <Redirect to="/dashboard" />
        } else {
            return <Redirect to="/verify" />
        }
    }

    if (mode === "Log in") {
        return (
            <div className="fractalContainer">
                <Header dark={false} />
                <LoginView />
            </div>
        )
    } else if (mode === "Sign up") {
        return (
            <div className="fractalContainer">
                <Header dark={false} />

                <SignupView />
            </div>
        )
    } else if (mode === "Forgot") {
        return (
            <div className="fractalContainer">
                <Header dark={false} />
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
        mode:
            state.AuthReducer.authFlow && state.AuthReducer.authFlow.mode
                ? state.AuthReducer.authFlow.mode
                : "Sign up",
        user: state.AuthReducer.user,
        authFlow: state.AuthReducer.authFlow,
    }
}

export default connect(mapStateToProps)(Auth)
