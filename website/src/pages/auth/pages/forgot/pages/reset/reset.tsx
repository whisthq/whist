/* eslint-disable no-console */

// npm imports
import React, {
    useState,
    useEffect,
    ChangeEvent,
    KeyboardEvent,
    Dispatch,
} from "react"
import { connect } from "react-redux"
import { useLocation } from "react-router-dom"

import { PuffAnimation } from "shared/components/loadingAnimations"
import PasswordConfirmForm from "shared/components/passwordConfirmForm"
import {
    checkPassword,
    checkPasswordVerbose,
} from "pages/auth/shared/helpers/authHelpers"
import history from "shared/utils/history"
import FractalKey from "shared/types/input"
import { AuthFlow } from "store/reducers/auth/default"

import { updateUser, updateAuthFlow } from "store/actions/auth/pure"
import {
    resetPassword,
    validateResetToken,
} from "store/actions/auth/sideEffects"
import { deepCopy } from "shared/utils/reducerHelpers"
import { DEFAULT } from "store/reducers/auth/default"
import { RESET_IDS } from "testing/utils/testIDs"

import styles from "styles/auth.module.css"
import sharedStyles from "styles/shared.module.css"

import AuthContainer from "pages/auth/shared/components/authContainer"

const Reset = (props: {
    dispatch: Dispatch<any>
    authFlow: AuthFlow
}) => {
    const { dispatch, authFlow } = props

    const [password, setPassword] = useState("")
    const [confirmPassword, setConfirmPassword] = useState("")
    const [passwordWarning, setPasswordWarning] = useState("")
    const [confirmPasswordWarning, setConfirmPasswordWarning] = useState("")
    const [processing, setProcessing] = useState(false)

    const accessToken = useLocation().search.substring(1, useLocation().search.length)

    // the actual logic
    const checkPasswords = () => {
        return checkPassword(password) && confirmPassword === password
    }

    const reset = () => {
        if (checkPasswords()) {
            dispatch(
                resetPassword(password)
            )
        }
    }

    const logout = () => {
        dispatch(updateUser(deepCopy(DEFAULT.user)))
        dispatch(updateAuthFlow(deepCopy(DEFAULT.authFlow)))
    }

    const changePassword = (evt: ChangeEvent<HTMLInputElement>) => {
        evt.persist()
        setPassword(evt.target.value)
    }

    const changeConfirmPassword = (evt: ChangeEvent<HTMLInputElement>) => {
        evt.persist()
        setConfirmPassword(evt.target.value)
    }

    // Handles ENTER key press
    const onKeyPress = (evt: KeyboardEvent) => {
        if (evt.key === FractalKey.ENTER) {
            reset()
        }
    }

    const backToLogin = () => {
        dispatch(updateUser(deepCopy(DEFAULT.user)))
        dispatch(updateAuthFlow(deepCopy(DEFAULT.authFlow)))
        history.push("/auth")
    }

    // first ask for a validation and start loading
    useEffect(() => {
        if (!processing && accessToken != null) {
            dispatch(
                updateAuthFlow({
                    resetTokenStatus: null,
                })
            )
            dispatch(validateResetToken(accessToken))
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

    if (processing) {
        return (
            <AuthContainer title="Please wait while we authenticate you">
                <div data-testid={RESET_IDS.LOAD}>
                    <PuffAnimation />
                </div>
            </AuthContainer>
        )
    } else if (authFlow.resetTokenStatus === "verified") {
        return (
           <AuthContainer title="Please enter a new password">
                    <div data-testid={RESET_IDS.FORM}>
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
                    </div>
                    <div data-testid={RESET_IDS.BUTTON}>
                        <button
                            className="rounded bg-blue dark:bg-mint px-8 py-3 mt-4 text-white dark:text-black w-full hover:bg-mint hover:text-black duration-500 font-medium"
                            onClick={reset}
                        >
                            Reset
                        </button>
                    </div>
            </AuthContainer>
        )
    } else {
        return (
            <AuthContainer title="Your password reset was unsuccessful">
                <div className="text-gray mt-4 text-center">
                    We apologize to the inconvenience!. Please contact support@fractal.co and we'll assist you in resetting your password.
                </div>
                <button
                    className="rounded bg-blue dark:bg-mint px-8 py-3 mt-6 text-white dark:text-black w-full hover:bg-mint hover:text-black duration-500 font-medium"
                    style={{
                        width: "100%",
                        fontSize: 16,
                    }}
                    onClick={backToLogin}
                >
                    Back to Login
                </button>
            </AuthContainer>
        )
    }
}

const mapStateToProps = (state: {
    AuthReducer: {
        authFlow: AuthFlow
    }
}) => {
    return {
        authFlow: state.AuthReducer.authFlow,
    }
}

export default connect(mapStateToProps)(Reset)
