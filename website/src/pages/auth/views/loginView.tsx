import React, { useState, useEffect } from "react"
import { connect } from "react-redux"
import { FormControl } from "react-bootstrap"
import PuffLoader from "react-spinners/PuffLoader"

import "styles/auth.css"

import Input from "shared/components/input"
import * as AuthSideEffect from "store/actions/auth/sideEffects"
import * as AuthPureAction from "store/actions/auth/pure"
import {
    loginEnabled,
    checkEmail,
    checkPassword,
} from "pages/auth/constants/authHelpers"
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

    useEffect(() => {
        setProcessing(false)
    }, [dispatch, user.user_id, authFlow])

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
                    css={
                        "position: absolute; top: 50%; left: 50%; transform: translate(-50%, -50%);"
                    }
                    size={75}
                />
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
                        marginTop: 120,
                    }}
                >
                    <h2
                        style={{
                            color: "#111111",
                            textAlign: "center",
                        }}
                    >
                        Welcome back! {props.user.user_id}
                    </h2>
                    <div style={{ marginTop: 40 }}>
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
                            marginTop: 15,
                            background: "#3930b8",
                            border: "none",
                            color: "white",
                            fontSize: 16,
                            paddingTop: 15,
                            paddingBottom: 15,
                            opacity: loginEnabled(email, password) ? 1.0 : 0.6,
                        }}
                        onClick={login}
                        disabled={!loginEnabled(email, password)}
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
                    ></div>
                    <GoogleButton />
                    <div style={{ textAlign: "center", marginTop: 20 }}>
                        Need to create an account?{" "}
                        <span
                            style={{ color: "#3930b8" }}
                            className="hover"
                            onClick={() =>
                                dispatch(
                                    AuthPureAction.updateAuthFlow({
                                        mode: "Sign up",
                                    })
                                )
                            }
                        >
                            Sign up
                        </span>{" "}
                        here.
                    </div>
                </div>
            </div>
        )
    }
}

function mapStateToProps(state: any) {
    console.log(state)
    return {
        user: state.AuthReducer.user,
        authFlow: state.AuthReducer.authFlow,
    }
}

export default connect(mapStateToProps)(LoginView)
