import React, { useState, useEffect } from "react"
import { connect } from "react-redux"

import "styles/auth.css"

import Input from "shared/components/input"

import * as AuthSideEffect from "store/actions/auth/sideEffects"
import * as AuthPureAction from "store/actions/auth/pure"
import { deepCopy } from "shared/utils/reducerHelpers"
import { DEFAULT } from "store/reducers/auth/default"

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
            dispatch(
                AuthPureAction.updateAuthFlow({
                    passwordResetEmail: email,
                })
            )
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

    const backToLogin = () => {
        const defaultAuthFlow = deepCopy(DEFAULT.authFlow)
        defaultAuthFlow.mode = "Log in"
        dispatch(AuthPureAction.updateAuthFlow(defaultAuthFlow))
    }

    useEffect(() => {
        if (authFlow.forgotStatus && authFlow.forgotEmailsSent) {
            setProcessing(false)
            setGotResponse(true)
        }
    }, [authFlow.forgotStatus, authFlow.forgotEmailsSent])

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
                <div className="auth-container">
                    <div className="auth-title">
                        {authFlow.forgotStatus ? authFlow.forgotStatus : ""}
.
</div>
                    <div
                        style={{
                            color: "#333333",
                            textAlign: "center",
                            marginTop: 20,
                        }}
                    >
                        Didn't receive an email? Please check your spam folder.
                        To receive another email, click
{" "}
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
                            onClick={backToLogin}
                        />
                    </div>
                </div>
            </div>
        )
    } else {
        return (
            <div>
                <div className="auth-container">
                    <div className="auth-title">
Enter your email.
</div>
                    <div style={{ marginTop: 40 }}>
                        <Input
                            text="Email"
                            type="email"
                            placeholder="bob@gmail.com"
                            onChange={changeEmail}
                            onKeyPress={onKeyPress}
                            value={email}
                            valid={checkEmail(email)}
                        />
                    </div>
                    <button
                        className="purple-button"
                        style={{
                            opacity: checkEmail(email) ? 1.0 : 0.6,
                        }}
                        onClick={forgot}
                        disabled={!checkEmail(email)}
                    >
                        Reset
                    </button>
                    <div className="line" />
                    <div style={{ marginTop: 20 }}>
                        <SwitchMode
                            question="Remember your password?"
                            link="Log in "
                            closer="here."
                            onClick={backToLogin}
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
