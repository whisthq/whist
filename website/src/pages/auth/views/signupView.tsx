import React, { useState, useEffect } from "react"
import { connect } from "react-redux"
import { FormControl } from "react-bootstrap"
import PuffLoader from "react-spinners/PuffLoader"

import "styles/auth.css"

// import MainContext from "shared/context/mainContext"
import * as AuthSideEffect from "store/actions/auth/sideEffects"
import * as AuthPureAction from "store/actions/auth/pure"

import GoogleButton from "pages/auth/components/googleButton"

const SignupView = (props: { dispatch: any; user: any; authFlow: any }) => {
    const { authFlow, user, dispatch } = props

    const [email, setEmail] = useState("")
    const [password, setPassword] = useState("")
    const [confirmPassword, setConfirmPassword] = useState("")

    const [processing, setProcessing] = useState(false)

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

    const signup = () => {
        if (email.length > 4 && password.length > 6 && email.includes("@")) {
            setProcessing(true)
            dispatch(AuthSideEffect.emailSignup(email, password))
        } else {
            if (email.length > 4 && email.includes("@")) {
                dispatch(
                    AuthPureAction.updateAuthFlow({
                        signupWarning: "Invalid email. Try a different email.",
                    })
                )
            } else {
                dispatch(
                    AuthPureAction.updateAuthFlow({
                        signupWarning:
                            "Password must be at least seven characters.",
                    })
                )
            }
        }
    }

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

    const loaderCSS =
        "position: absolute; top: 50%; left: 50%; transform: translate(-50%, -50%);"

    useEffect(() => {
        setProcessing(false)
    }, [user.user_id, authFlow])

    if (processing) {
        return (
            <div
                style={{
                    width: "100vw",
                    height: "100vh",
                    position: "relative",
                }}
            >
                <PuffLoader css={loaderCSS} size={75} />
            </div>
        )
    } else {
        return (
            <div>
                <div
                    style={{
                        width: 400,
                        margin: "auto",
                        marginTop: 150,
                    }}
                >
                    <h2
                        style={{
                            color: "#111111",
                            textAlign: "center",
                        }}
                    >
                        Let's get started.
                    </h2>
                    <FormControl
                        type="email"
                        aria-label="Default"
                        aria-describedby="inputGroup-sizing-default"
                        placeholder="Email Address"
                        className="input-form"
                        onChange={changeEmail}
                        onKeyPress={onKeyPress}
                        value={email}
                        style={{
                            marginTop: 40,
                        }}
                    />
                    <FormControl
                        type="password"
                        aria-label="Default"
                        aria-describedby="inputGroup-sizing-default"
                        placeholder="Password"
                        className="input-form"
                        onChange={changePassword}
                        onKeyPress={onKeyPress}
                        value={password}
                        style={{
                            marginTop: 15,
                        }}
                    />
                    <FormControl
                        type="password"
                        aria-label="Default"
                        aria-describedby="inputGroup-sizing-default"
                        placeholder="Confirm Password"
                        className="input-form"
                        onChange={changeConfirmPassword}
                        onKeyPress={onKeyPress}
                        value={confirmPassword}
                        style={{
                            marginTop: 15,
                        }}
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
                            paddingTop: 20,
                            paddingBottom: 20,
                        }}
                        onClick={signup}
                    >
                        Sign up
                    </button>
                    <div
                        style={{
                            height: 1,
                            width: "100%",
                            marginTop: 30,
                            marginBottom: 30,
                            background: "#EFEFEF",
                        }}
                    ></div>
                    <GoogleButton />
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
