import React, { useState, useEffect } from "react"
import { connect } from "react-redux"

import Input from "shared/components/input"

import * as AuthSideEffect from "store/actions/auth/sideEffects"
import * as AuthPureAction from "store/actions/auth/pure"

import { checkEmail } from "pages/auth/constants/authHelpers"
import SwitchMode from "pages/auth/components/switchMode"
import { PagePuff } from "shared/components/loadingAnimations"

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
        forgot() // note check happens inside forgot
    }

    useEffect(() => {
        setProcessing(false)
        setGotResponse(true)
    }, [authFlow.forgotStatus, authFlow.forgotEmailsSent])

    if (processing) {
        return (
            <div
                style={{
                    width: "100vw",
                    height: "100vh",
                    position: "relative",
                }}
            >
                <PagePuff />
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
                        Failed
                        {authFlow.forgotStatus
                            ? " " + authFlow.forgotStatus
                            : ""}
                        .
                    </h2>
                    <div
                        style={{
                            color: "#fc3d03",
                            textAlign: "center",
                        }}
                    >
                        We've sent you {authFlow.forgotEmailsSent} reset emails.
                    </div>
                    <SwitchMode
                        question="Try going back to login"
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
                    <SwitchMode
                        question="Remembered your password?"
                        link="Log in"
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
        )
    }
}

function mapStateToProps(state: any) {
    return {
        authFlow: state.AuthReducer.authFlow,
    }
}

export default connect(mapStateToProps)(ForgotView)
