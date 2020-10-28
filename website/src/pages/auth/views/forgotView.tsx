import React, { useState, useEffect } from "react"
import { connect } from "react-redux"

import Input from "shared/components/input"

import * as AuthSideEffect from "store/actions/auth/sideEffects"
import * as AuthPureAction from "store/actions/auth/pure"

import { checkEmail } from "pages/auth/constants/authHelpers"
import SwitchMode from "pages/auth/components/switchMode"
import { PuffAnimation } from "shared/components/loadingAnimations"

const ForgotView = (props: any) => {
    const { authFlow, dispatch } = props

    const [email, setEmail] = useState("")
    const [processing, setProcessing] = useState(false)
    const [gotResponse, setGotResponse] = useState(false)

    const forgot = () => {
        if (checkEmail(email)) {
            setProcessing(true)
            dispatch(AuthSideEffect.forgotPassword(email))
        }
    }

    const changeEmail = (evt: any): any => {
        evt.persist()
        setEmail(evt.target.value)
    }

    const onKeyPress = (evt: any) => {
        if (evt.key === "Enter") {
            forgot() // note check happens inside forgot
        }
    }

    useEffect(() => {
        // this should not be called on component mount
        if (email && authFlow.forgotStatus && authFlow.forgotEmailsSent) {
            setProcessing(false)
            setGotResponse(true)
        }
    }, [authFlow.forgotStatus, authFlow.forgotEmailsSent, email])

    // useEffect(() => {
    //     setGotResponse(false)
    // }, [])

    if (processing) {
        return (
            <div>
                <PuffAnimation />
            </div>
        )
    } else if (gotResponse) {
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
                        {authFlow.forgotStatus &&
                        authFlow.forgotStatus !== "Email sent"
                            ? "Failed"
                            : "Succeded"}
                        {authFlow.forgotStatus ? authFlow.forgotStatus : ""}.
                    </h2>
                    <div
                        style={{
                            color: "#333333",
                            textAlign: "center",
                            marginTop: 20,
                        }}
                    >
                        Didn't receive an email? Please check your spam folder.
                        To receive another email, click{" "}
                        <span
                            onClick={() => setGotResponse(false)}
                            style={{ color: "#3930b8", cursor: "pointer" }}
                        >
                            here
                        </span>
                        .
                    </div>
                    <div style={{ marginTop: 20 }}>
                        <SwitchMode
                            question="You can return to the login page"
                            link="here"
                            closer="."
                            onClick={() =>
                                dispatch(
                                    AuthPureAction.updateAuthFlow({
                                        mode: "Log in",
                                    })
                                )
                            }
                        />
                    </div>
                </div>
            </div>
        )
    } else {
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
                        Enter your email.
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
                            opacity: checkEmail(email) ? 1.0 : 0.6,
                        }}
                        onClick={forgot}
                        disabled={!checkEmail(email)}
                    >
                        Reset
                    </button>
                    <div style={{ marginTop: 20 }}>
                        <SwitchMode
                            question="Remember your password?"
                            link="Log in "
                            closer="here."
                            onClick={() =>
                                dispatch(
                                    AuthPureAction.updateAuthFlow({
                                        mode: "Log in",
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
    return {
        authFlow: state.AuthReducer.authFlow,
    }
}

export default connect(mapStateToProps)(ForgotView)
