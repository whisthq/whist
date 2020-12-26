import React, { useState, useEffect } from "react"
import { connect } from "react-redux"

import { PuffAnimation } from "shared/components/loadingAnimations"
import PasswordConfirmForm from "shared/components/passwordConfirmForm"
import {
    checkPassword,
    checkPasswordVerbose,
} from "pages/auth/constants/authHelpers"
import history from "shared/utils/history"

import { updateUser, updateAuthFlow } from "store/actions/auth/pure"
import {
    resetPassword,
    validateResetToken,
} from "store/actions/auth/sideEffects"
import { deepCopy } from "shared/utils/reducerHelpers"
import { DEFAULT } from "store/reducers/auth/default"

import "styles/auth.css"

const ResetView = (props: {
    dispatch: any
    user: any
    authFlow: any
    token?: any
    validToken?: boolean
}) => {
    const { dispatch, authFlow, token, validToken } = props

    const [password, setPassword] = useState("")
    const [passwordWarning, setPasswordWarning] = useState("")
    const [confirmPassword, setConfirmPassword] = useState("")
    const [confirmPasswordWarning, setConfirmPasswordWarning] = useState("")
    const [finished, setFinished] = useState(false)

    // visual state constants
    const [processing, setProcessing] = useState(false)

    // the actual logic
    const validPassword =
        password.length > 0 &&
        confirmPassword === password &&
        checkPassword(password)

    const reset = () => {
        if (validPassword) {
            // TODO (might also want to add a email redux state for that which was forgotten)
            dispatch(
                resetPassword(
                    authFlow.passwordResetEmail,
                    password,
                    authFlow.passwordResetToken
                )
            )
            setFinished(true) // unfortunately this is all the sagas give us
        }
    }

    const logout = () => {
        dispatch(updateUser(deepCopy(DEFAULT.user)))
        dispatch(updateAuthFlow(deepCopy(DEFAULT.authFlow)))
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
        if (evt.key === "Enter") {
            reset()
        }
    }

    const backToLogin = () => {
        dispatch(updateUser(deepCopy(DEFAULT.user)))
        dispatch(updateAuthFlow(deepCopy(DEFAULT.authFlow)))
        history.push("/auth/bypass")
    }

    // first ask for a validation and start loading
    useEffect(() => {
        if (validToken && !processing) {
            dispatch(
                updateAuthFlow({
                    resetTokenStatus: null,
                })
            )
            dispatch(validateResetToken(token))
            setProcessing(true)
        }
        // want onComponentMount basically (thus [] ~ no deps ~ called only at the very beginning)
        // eslint-disable-next-line react-hooks/exhaustive-deps
    }, [])

    useEffect(() => {
        if (processing && authFlow.resetTokenStatus) {
            setProcessing(false)
        }
        // needs to be triggered by a change in resetVerificationsDone and not authFlow
        // eslint-disable-next-line react-hooks/exhaustive-deps
    }, [authFlow.resetTokenStatus])

    // textbox stuff
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

    // finally once they click send dispatch a reset as in the beginning and finished -> true
    useEffect(() => {
        if (finished) {
            // delay for 3 seconds then push to /
            // not sure how this compares to redirect or whatever
            setTimeout(() => {
                logout()
                history.push("/auth/bypass")
            }, 5000) // turn this into a helper?
        }
        // eslint-disable-next-line react-hooks/exhaustive-deps
    }, [finished])

    if (finished) {
        // assume it worked
        // TODO (adriano) when the server actually responds do something about it (say your thing was reset for example)
        return (
            <div>
                <PuffAnimation />
            </div>
        )
    } else if (processing) {
        return (
            <div>
                <PuffAnimation />
            </div>
        )
    } else if (authFlow.resetTokenStatus === "verified") {
        return (
            <div>
                <div className="auth-container">
                    <div className="auth-title">
                        Please enter your new password.
                    </div>
                    <PasswordConfirmForm
                        changePassword={changePassword}
                        changeConfirmPassword={changeConfirmPassword}
                        onKeyPress={onKeyPress}
                        password={password}
                        confirmPassword={confirmPassword}
                        passwordWarning={passwordWarning}
                        confirmPasswordWarning={confirmPasswordWarning}
                        isFirstElement={true}
                    />
                    <button
                        className="purple-button"
                        style={{
                            opacity: validPassword ? 1.0 : 0.6,
                        }}
                        onClick={reset}
                        disabled={!validPassword}
                    >
                        Reset
                    </button>
                </div>
            </div>
        )
    } else {
        return (
            <div>
                <div className="auth-container">
                    <div className="auth-title">Failed to verify token.</div>
                    <button
                        className="white-button"
                        style={{
                            width: "100%",
                            fontSize: 16,
                        }}
                        onClick={backToLogin}
                    >
                        Back to Login
                    </button>
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

export default connect(mapStateToProps)(ResetView)
