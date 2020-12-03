import React, { useEffect, useState } from "react"
import { connect } from "react-redux"
import { Redirect } from "react-router"

import Header from "shared/components/header"
import LoginView from "pages/auth/views/loginView"
import SignupView from "pages/auth/views/signupView"
import ForgotView from "pages/auth/views/forgotView"

import "styles/auth.css"

import history from "shared/utils/history"
import { updateUser, updateAuthFlow } from "store/actions/auth/pure"

const Auth = (props: {
    dispatch: any
    user: {
        userID: string
        emailVerified: boolean
        canLogin: boolean
        accessToken: string
    }
    waitlistUser: {
        userID: string
    }
    mode: any
    authFlow: {
        callback: string
    }
    match: any
    location: any
    window: any
}) => {
    const { user, waitlistUser, match, mode, authFlow, dispatch } = props

    const [redirectToCallback, setRedirectToCallback] = useState(false)
    const [initialCallback, setInitialCallback] = useState("")
    const [callbackChecked, setCallbackChecked] = useState(false)

    useEffect(() => {
        // Read callback search param
        const qs = require("qs")
        const callback = qs.parse(props.location.search, {
            ignoreQueryPrefix: true,
        }).callback

        // Clear any previous callback URL
        dispatch(updateAuthFlow({ callback: undefined }))

        // Save new callback to local state, if any
        if (callback && callback !== "") {
            setInitialCallback(callback)
        }

        setCallbackChecked(true)

        // Read other params
        const firstParam = match.params.first
        const secondParam = match.params.second

        if (firstParam !== "bypass" && !waitlistUser.userID) {
            history.push("/")
        }

        if (secondParam && secondParam !== "") {
            dispatch(updateUser({ waitlistToken: secondParam }))
        }
    }, [match, waitlistUser.userID, dispatch, props.location.search])

    // If a callback was provided, save it to Redux
    useEffect(() => {
        if (initialCallback && initialCallback !== "") {
            if (
                initialCallback.includes("fractal://auth") &&
                user.accessToken
            ) {
                const finalCallback =
                    initialCallback + "?accessToken=" + user.accessToken
                dispatch(updateAuthFlow({ callback: finalCallback }))
            } else if (!initialCallback.includes("fractal://auth")) {
                dispatch(updateAuthFlow({ callback: initialCallback }))
            }
        }
    }, [authFlow.callback, user.accessToken, dispatch, initialCallback])

    // If Redux callback found and we have not redirected to the callback yet, then redirect
    useEffect(() => {
        if (
            !redirectToCallback &&
            authFlow.callback &&
            authFlow.callback !== "" &&
            initialCallback &&
            initialCallback !== "" &&
            user.accessToken &&
            window
        ) {
            window.location.replace(authFlow.callback)
            setRedirectToCallback(true)
        }
    }, [
        authFlow.callback,
        initialCallback,
        redirectToCallback,
        user.accessToken,
    ])

    if (redirectToCallback) {
        return <Redirect to="/callback" />
    }

    if (user.userID && user.userID !== "") {
        if (user.emailVerified && initialCallback === "" && callbackChecked) {
            return <Redirect to="/dashboard" />
        } else if (
            !user.emailVerified &&
            initialCallback === "" &&
            callbackChecked
        ) {
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
