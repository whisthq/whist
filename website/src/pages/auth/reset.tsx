import React, { useState, useEffect } from "react"
import { connect } from "react-redux"
import { Redirect, useLocation } from "react-router-dom"
import {
    resetPassword,
    validateResetToken,
} from "store/actions/auth/sideEffects"

// TODO we always use more or less the same looking puff loader
// let's put it in shared or something
import PuffLoader from "react-spinners/PuffLoader"

import {
    checkPassword,
    checkPasswordVerbose,
} from "pages/auth/constants/authHelpers"

import Input from "shared/components/input"

import "styles/auth.css"

// props type
// {
//     dispatch: any
// }
const Reset = (props: {
    dispatch: any
    user: {
        user_id: string
    }
    authFlow: {
        resetTokenStatus: string | null
        resetDone: boolean
    }
}) => {
    const { dispatch, user, authFlow } = props

    // token constants
    const search = useLocation().search
    const token = search.substring(1, search.length)

    // password constants
    // TODO we should really make a generic form component that we can modify into a
    // specialized form component
    const [password, setPassword] = useState("")
    const [passwordWarning, setPasswordWarning] = useState("")
    const [confirmPassword, setConfirmPassword] = useState("")
    const [confirmPasswordWarning, setConfirmPasswordWarning] = useState("")

    // visual state constants
    const [processing, setProcessing] = useState(false)

    // the actual logic
    const valid_token = token && token.length >= 1
    const valid_password =
        password.length > 0 &&
        confirmPassword === password &&
        checkPassword(password)

    const reset = () => {
        // TODO (might also want to add a email redux state for that which was forgotten)
        dispatch(resetPassword(user.user_id, password))
    }

    const changePassword = (evt: any): any => {
        evt.persist()
        setPassword(evt.target.value)
    }

    const changeConfirmPassword = (evt: any): any => {
        evt.persist()
        setConfirmPassword(evt.target.value)
    }

    // Handles ENTER key press
    const onKeyPress = (evt: any) => {
        if (evt.key === "Enter" && valid_password) {
            reset()
        }
    }

    // password warnings
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

    useEffect(() => {
        if (valid_token && !processing) {
            dispatch(validateResetToken(token))

            setProcessing(true)
        }
        // want onComponentMount basically (thus [] ~ no deps ~ called only at the very beginning)
        // eslint-disable-next-line react-hooks/exhaustive-deps
    }, [])

    useEffect(() => {
        if (authFlow.resetTokenStatus === "verified" && processing) {
            setProcessing(false)
        }
        // eslint-disable-next-line react-hooks/exhaustive-deps
    }, [authFlow.resetTokenStatus])

    // return tsx
    if (!valid_token || authFlow.resetDone) {
        return <Redirect to="/" />
    } else if (processing) {
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
    } else if (authFlow.resetTokenStatus === "verified") {
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
                        Please Enter Your New Password {props.user.user_id}
                    </h2>
                    <div style={{ marginTop: 40 }}>
                        <Input
                            text="Email"
                            type="email"
                            placeholder="bob@tryfractal.com"
                            onChange={changePassword}
                            onKeyPress={onKeyPress}
                            value={password}
                            warning={passwordWarning}
                            valid={checkPassword(password)}
                        />
                    </div>
                    <div style={{ marginTop: 13 }}>
                        <Input
                            text="Confirm Password"
                            type="password"
                            placeholder="Password"
                            onChange={changeConfirmPassword}
                            onKeyPress={onKeyPress}
                            value={confirmPassword}
                            warning={confirmPasswordWarning}
                            valid={valid_password}
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
                            opacity: valid_password ? 1.0 : 0.6,
                        }}
                        onClick={reset}
                        disabled={!valid_password}
                    >
                        Reset
                    </button>
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
                        Failed to verify token.
                    </h2>
                </div>
            </div>
        )
    }
}

function mapStateToProps(state: { AuthReducer: { user: any; authFlow: any } }) {
    return {
        user: state.AuthReducer.user ? state.AuthReducer.user : {},
        authFlow: state.AuthReducer.authFlow ? state.AuthReducer.authFlow : {},
    }
}

export default connect(mapStateToProps)(Reset)
