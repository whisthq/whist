import React, { useState, useEffect } from "react"
import { connect } from "react-redux"
import PuffLoader from "react-spinners/PuffLoader"

import "styles/auth.css"

// import MainContext from "shared/context/mainContext"
import Input from "shared/components/input"
import * as AuthSideEffect from "store/actions/auth/sideEffects"
import * as AuthPureAction from "store/actions/auth/pure"
import {
    checkPasswordVerbose,
    checkEmailVerbose,
    signupEnabled,
    checkEmail,
} from "pages/auth/constants/authHelpers"

import PasswordConfirmForm from "pages/auth/components/passwordConfirmForm"

import GoogleButton from "pages/auth/components/googleButton"

const SignupView = (props: { dispatch: any; user: any; authFlow: any }) => {
    const { authFlow, user, dispatch } = props

    const [email, setEmail] = useState("")
    const [password, setPassword] = useState("")
    const [confirmPassword, setConfirmPassword] = useState("")
    const [emailWarning, setEmailWarning] = useState("")
    const [passwordWarning, setPasswordWarning] = useState("")
    const [confirmPasswordWarning, setConfirmPasswordWarning] = useState("")

    const [processing, setProcessing] = useState(false)

    // Dispatches signup API call
    const signup = () => {
        if (signupEnabled(email, password, confirmPassword)) {
            setProcessing(true)
            dispatch(AuthSideEffect.emailSignup(email, password))
        }
    }

    // so we can display puff while server does it's thing for google as well
    const google_signup = (code: any) => {
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
            dispatch(AuthSideEffect.emailSignup(email, password))
        }
    }

    // Updates email, password, confirm password fields as user types
    const changeEmail = (evt: any): any => {
        evt.persist()
        setEmail(evt.target.value)
    }

    const changePassword = (evt: any): any => {
        evt.persist()
        setPassword(evt.target.value)
    }

    const changeConfirmPassword = (evt: any): any => {
        evt.persist()
        setConfirmPassword(evt.target.value)
    }

    // Removes loading screen on prop change and page load
    useEffect(() => {
        setProcessing(false)
    }, [user.user_id, authFlow])

    // Email and password dynamic warnings
    useEffect(() => {
        setEmailWarning(checkEmailVerbose(email))
    }, [email])

    useEffect(() => {
        setPasswordWarning(checkPasswordVerbose(password))
    }, [password])

    useEffect(() => {
        if (
            confirmPassword !== password &&
            password.length > 0 &&
            confirmPassword.length > 0
        ) {
            setConfirmPasswordWarning("Doesn't match")
        } else {
            setConfirmPasswordWarning("")
        }
        // we only want to change on a specific state change
        // eslint-disable-next-line react-hooks/exhaustive-deps
    }, [confirmPassword])

    if (processing) {
        // Conditionally render the loading screen as we wait for signup API call to return
        return (
            <div
                style={{
                    width: "100vw",
                    height: "100vh",
                    position: "relative",
                }}
            >
                <PuffLoader
                    css="position: absolute; top: 50%; left: 50%; transform: translate(-50%, -50%);"
                    size={75}
                />
            </div>
        )
    } else {
        // Render the signup screen
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
                        }}
                    >
                        Let's get started.
                    </div>
                    <div style={{ marginTop: 40 }}>
                        <Input
                            text="Email"
                            type="email"
                            placeholder="bob@tryfractal.com"
                            onChange={changeEmail}
                            onKeyPress={onKeyPress}
                            value={email}
                            warning={emailWarning}
                            valid={checkEmail(email)}
                        />
                    </div>
                    <PasswordConfirmForm
                        changePassword={changePassword}
                        changeConfirmPassword={changeConfirmPassword}
                        onKeyPress={onKeyPress}
                        password={password}
                        confirmPassword={confirmPassword}
                        passwordWarning={passwordWarning}
                        confirmPasswordWarning={confirmPasswordWarning}
                        isFirstElement={false}
                    />
                    <button
                        className="white-button"
                        style={{
                            width: "100%",
                            marginTop: 15,
                            background: "#3930b8",
                            border: "none",
                            color: "white",
                            fontSize: 16,
                            paddingTop: 15,
                            paddingBottom: 15,
                            opacity: signupEnabled(
                                email,
                                password,
                                confirmPassword
                            )
                                ? 1.0
                                : 0.6,
                        }}
                        onClick={signup}
                        disabled={
                            !signupEnabled(email, password, confirmPassword)
                        }
                    >
                        Sign up
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
                    <GoogleButton login={google_signup} />
                    <div style={{ textAlign: "center", marginTop: 20 }}>
                        Already have an account?{" "}
                        <span
                            style={{ color: "#3930b8" }}
                            className="hover"
                            onClick={() =>
                                dispatch(
                                    AuthPureAction.updateAuthFlow({
                                        mode: "Log in",
                                    })
                                )
                            }
                        >
                            Log in
                        </span>{" "}
                        here.
                    </div>
                </div>
            </div>
        )
    }
}

function mapStateToProps(state: { AuthReducer: { user: any; authFlow: any } }) {
    return {
        user: state.AuthReducer.user,
        authFlow: state.AuthReducer.authFlow,
    }
}

export default connect(mapStateToProps)(SignupView)
