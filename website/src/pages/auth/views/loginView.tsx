import React, { useState, useEffect } from "react"
import { connect } from "react-redux"
import { PuffAnimation } from "shared/components/loadingAnimations"

import "styles/auth.css"

import Input from "shared/components/input"
import * as AuthSideEffect from "store/actions/auth/sideEffects"
import * as AuthPureAction from "store/actions/auth/pure"
import {
    loginEnabled,
    checkEmail,
    checkPassword,
} from "pages/auth/constants/authHelpers"
import SwitchMode from "pages/auth/components/switchMode"
import GoogleButton from "pages/auth/components/googleButton"

const LoginView = (props: any) => {
    const { dispatch, user, authFlow } = props

    const [email, setEmail] = useState("")
    const [password, setPassword] = useState("")
    const [processing, setProcessing] = useState(false)

    // Dispatches the login API call
    const login = () => {
        if (loginEnabled(email, password)) {
            setProcessing(true)
            dispatch(AuthSideEffect.emailLogin(email, password))
        }
    }

    // so we can display puff while server does it's thing for google as well
    const google_login = (code: any) => {
        setProcessing(true)
        dispatch(AuthSideEffect.googleLogin(code))
    }

    // Handles ENTER key press
    const onKeyPress = (evt: any) => {
        if (
            evt.key === "Enter" &&
            email.length > 4 &&
            password.length > 6 &&
            email.includes("@")
        ) {
            setProcessing(true)
            dispatch(AuthSideEffect.emailLogin(email, password))
        }
    }

    // Updates email and password fields as user types
    const changeEmail = (evt: any): any => {
        evt.persist()
        setEmail(evt.target.value)
    }

    const changePassword = (evt: any): any => {
        evt.persist()
        setPassword(evt.target.value)
    }

    // should trigger when they successfully log in... be it with google or with email
    useEffect(() => {
        setProcessing(false)
    }, [dispatch, user.user_id, authFlow])

    if (processing) {
        // Conditionally render the loading screen as we wait for signup API call to return
        return (
            <div>
                <PuffAnimation />
            </div>
        )
    } else {
        // Render the login screen
        return (
            <div>
                <div
                    style={{
                        width: 400,
                        margin: "auto",
                        marginTop: 70,
                    }}
                >
                    <div
                        style={{
                            color: "#111111",
                            textAlign: "center",
                            fontSize: 32,
                            marginBottom: 40,
                        }}
                    >
                        Welcome back!
                    </div>
                    {authFlow.loginWarning && authFlow.loginWarning !== "" && (
                        <div
                            style={{
                                width: "100%",
                                background: "#ff5627",
                                padding: "15px 20px",
                                color: "white",
                                fontSize: 14,
                                marginTop: 5,
                                marginBottom: 20,
                            }}
                        >
                            {authFlow.loginWarning}
                        </div>
                    )}
                    <div style={{ marginTop: 13 }}>
                        <Input
                            text="Email"
                            type="email"
                            placeholder="bob@tryfractal.com"
                            onChange={changeEmail}
                            onKeyPress={onKeyPress}
                            value={email}
                            valid={checkEmail(email)}
                        />
                    </div>
                    <div style={{ marginTop: 13 }}>
                        <Input
                            text="Password"
                            altText={
                                <SwitchMode
                                    question=""
                                    link="Forgot password"
                                    closer=""
                                    onClick={() =>
                                        dispatch(
                                            AuthPureAction.updateAuthFlow({
                                                mode: "Forgot",
                                            })
                                        )
                                    }
                                />
                            }
                            type="password"
                            placeholder="Password"
                            onChange={changePassword}
                            onKeyPress={onKeyPress}
                            value={password}
                            valid={checkPassword(password)}
                        />
                    </div>
                    <button
                        className="white-button"
                        style={{
                            width: "100%",
                            marginTop: 20,
                            background: "#3930b8",
                            border: "none",
                            color: "white",
                            fontSize: 16,
                            paddingTop: 15,
                            paddingBottom: 15,
                            opacity: loginEnabled(email, password) ? 1.0 : 0.6,
                        }}
                        onClick={login}
<<<<<<< HEAD
                        disabled={!loginEnabled(email, password)}
=======
>>>>>>> staging
                    >
                        Log In
                    </button>
                    <div
                        style={{
                            height: 1,
                            width: "100%",
                            marginTop: 30,
                            marginBottom: 30,
                            background: "#dfdfdf",
                        }}
                    />
                    <GoogleButton login={google_login} />
                    <div style={{ marginTop: 20 }}>
                        <SwitchMode
                            question="Need to create an account?"
                            link="Sign up "
                            closer="here."
                            onClick={() =>
                                dispatch(
                                    AuthPureAction.updateAuthFlow({
                                        mode: "Sign up",
                                    })
                                )
                            }
                        />
                    </div>
                </div>
            </div>
        )
    }
}

function mapStateToProps(state: any) {
<<<<<<< HEAD
    console.log(state)
=======
>>>>>>> staging
    return {
        user: state.AuthReducer.user,
        authFlow: state.AuthReducer.authFlow,
    }
}

export default connect(mapStateToProps)(LoginView)
