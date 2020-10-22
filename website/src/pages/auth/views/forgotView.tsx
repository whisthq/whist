import React, { useState, useEffect } from "react"
import { connect } from "react-redux"

import Input from "shared/components/input"

import * as AuthSideEffect from "store/actions/auth/sideEffects"

import { checkEmail } from "pages/auth/constants/authHelpers"

import PuffLoader from "react-spinners/PuffLoader"

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
                <PuffLoader
                    css={
                        "position: absolute; top: 50%; left: 50%; transform: translate(-50%, -50%);"
                    }
                    size={75}
                />
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
                        Failed: {authFlow.forgotStatus}.
                    </h2>
                    <div>You've sent {authFlow.forgotEmailsSent}</div>
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
                        Enter Your Password {props.user.user_id}
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
