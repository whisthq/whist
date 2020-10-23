import React, { useState, useEffect } from "react"
import { connect } from "react-redux"
import { PagePuff } from "shared/components/loadingAnimations"
import PasswordConfirmForm from "pages/auth/components/passwordConfirmForm"
import {
    resetPassword,
    validateResetToken,
} from "store/actions/auth/sideEffects"
import {
    checkPassword,
    checkPasswordVerbose,
} from "pages/auth/constants/authHelpers"
import history from "shared/utils/history"

import "styles/auth.css"

const ResetView = (props: {
    dispatch: any
    user: any
    authFlow: any
    token?: any
    validToken?: boolean
}) => {
    const { dispatch, user, authFlow, token, validToken } = props

    const [password, setPassword] = useState("")
    const [passwordWarning, setPasswordWarning] = useState("")
    const [confirmPassword, setConfirmPassword] = useState("")
    const [confirmPasswordWarning, setConfirmPasswordWarning] = useState("")
    const [finished, setFinished] = useState(true)

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
            dispatch(resetPassword(user.user_id, password))
            setFinished(true) // unfortunately this is all the sagas give us
        }
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

    // first ask for a validation and start loading
    useEffect(() => {
        if (validToken && !processing) {
            dispatch(validateResetToken(token))

            setProcessing(true)
        }
        // want onComponentMount basically (thus [] ~ no deps ~ called only at the very beginning)
        // eslint-disable-next-line react-hooks/exhaustive-deps
    }, [])

    // then stop loading and let the textbox be displayed
    useEffect(() => {
        if (authFlow.resetTokenStatus === "verified" && processing) {
            setProcessing(false)
        }
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
            setTimeout(() => history.push("/auth"), 3000) // turn this into a helper?
        }
    }, [finished])

    if (finished) {
        // assume it worked
        // TODO (adriano) when the server actually responds do something about it (say your thing was reset for example)
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
                        Verified. You will soon be redirected.
                    </h2>
                </div>
            </div>
        )
    } else if (processing) {
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

export default connect(mapStateToProps)(ResetView)
