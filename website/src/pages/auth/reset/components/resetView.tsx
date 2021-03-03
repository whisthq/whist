/* eslint-disable no-console */
import React, {
    useState,
    useEffect,
    ChangeEvent,
    KeyboardEvent,
    Dispatch,
} from "react"
import { connect } from "react-redux"

import { PuffAnimation } from "shared/components/loadingAnimations"
import PasswordConfirmForm from "shared/components/passwordConfirmForm"
import {
    checkPassword,
    checkPasswordVerbose,
} from "pages/auth/constants/authHelpers"
import history from "shared/utils/history"
import FractalKey from "shared/types/input"
import { User, AuthFlow } from "shared/types/reducers"

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

import AuthContainer from "pages/auth/components/authContainer"

const ResetView = (props: {
    dispatch: Dispatch<any>
    user: User
    authFlow: AuthFlow
    token?: string
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
            let passwordResetEmail: string | undefined
            if (authFlow.passwordResetEmail == null) {
                console.error("Error: no passwordResetEmail")
            } else {
                passwordResetEmail = authFlow.passwordResetEmail as string
            }

            let passwordResetToken: string | undefined
            if (authFlow.passwordResetToken == null) {
                console.error("Error: no passwordResetToken")
            } else {
                passwordResetToken = authFlow.passwordResetToken as string
            }

            // TODO (might also want to add a email redux state for that which was forgotten)
            dispatch(
                resetPassword(password, passwordResetToken, passwordResetEmail)
            )
            setFinished(true) // unfortunately this is all the sagas give us
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
        if (validToken && !processing && token != null) {
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
                history.push("/auth")
            }, 5000) // turn this into a helper?
        }
        // eslint-disable-next-line react-hooks/exhaustive-deps
    }, [finished])

    if (finished) {
        // assume it worked
        // TODO (adriano) when the server actually responds do something about it (say your thing was reset for example)
        return (
            <AuthContainer title="Your password was reset successfully">
                <div data-testid={RESET_IDS.SUCCESS}>
                    <PuffAnimation />
                </div>
            </AuthContainer>
        )
    } else if (processing) {
        return (
            <AuthContainer title="Please wait, resetting your password">
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
                            style={{
                                opacity: validPassword ? 1.0 : 0.6,
                            }}
                            onClick={reset}
                            disabled={!validPassword}
                        >
                            Reset
                        </button>
                    </div>
            </AuthContainer>
        )
    } else {
        return (
            <AuthContainer title="Your password reset was unsuccessful">
                        <button
                            className="rounded bg-blue dark:bg-mint px-8 py-3 mt-4 text-white dark:text-black w-full hover:bg-mint hover:text-black duration-500 font-medium"
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
        user: User
        authFlow: AuthFlow
    }
}) => {
    return {
        user: state.AuthReducer.user ? state.AuthReducer.user : {},
        authFlow: state.AuthReducer.authFlow ? state.AuthReducer.authFlow : {},
    }
}

export default connect(mapStateToProps)(ResetView)
