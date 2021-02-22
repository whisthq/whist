import React, {
    useState,
    useEffect,
    KeyboardEvent,
    ChangeEvent,
    Dispatch,
} from "react"
import { connect } from "react-redux"

import Input from "shared/components/input"
import FractalKey from "shared/types/input"
import { AuthFlow } from "shared/types/reducers"

import * as AuthSideEffect from "store/actions/auth/sideEffects"
import * as AuthPureAction from "store/actions/auth/pure"
import { deepCopy } from "shared/utils/reducerHelpers"
import { DEFAULT } from "store/reducers/auth/default"

import { checkEmail } from "pages/auth/constants/authHelpers"
import SwitchMode from "pages/auth/components/switchMode"
import { PuffAnimation } from "shared/components/loadingAnimations"
import { AUTH_IDS, E2E_AUTH_IDS } from "testing/utils/testIDs"
import PLACEHOLDER from "shared/constants/form"

import styles from "styles/auth.module.css"

const Forgot = (props: {
    dispatch: Dispatch<any>
    authFlow: AuthFlow
    emailToken?: string
}) => {
    /*
        Component for when the user forgets their login information.

        Arguments:
            dispatch (Dispatch<any>): Action dispatcher
            user (User): User from Redux state
            authFlow (AuthFlow): AuthFlow from Redux state
    */
    const { authFlow, emailToken, dispatch } = props

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
            dispatch(AuthSideEffect.forgotPassword(email, emailToken))
        }
    }

    const changeEmail = (evt: ChangeEvent<HTMLInputElement>) => {
        evt.persist()
        setEmail(evt.target.value)
    }

    const onKeyPress = (evt: KeyboardEvent) => {
        if (evt.key === FractalKey.ENTER) {
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
            <div data-testid={AUTH_IDS.LOADING}>
                <PuffAnimation />
            </div>
        )
    } else if (gotResponse) {
        // for testing purposes, only to get the reset password token to navigate
        if (emailToken && emailToken?.length > 0) {
            return (
                <div>
                    <p data-testid={AUTH_IDS.RESETTOKEN}>{authFlow.token}</p>
                    <p data-testid={AUTH_IDS.RESETURL}>{authFlow.url}</p>
                </div>
            )
        }
        return (
            <div>
                <div className={styles.authContainer}>
                    <div className={styles.authTitle}>
                        {authFlow.forgotStatus ? authFlow.forgotStatus : ""}.
                    </div>
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
                    <div data-testid={AUTH_IDS.SWITCH}>
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
            </div>
        )
    } else {
        return (
            <div>
                <div className={styles.authContainer}>
                    <div className={styles.authTitle}>Enter your email.</div>
                    <div data-testid={AUTH_IDS.FORM}>
                        <div style={{ marginTop: 40 }}>
                            <Input
                                id={E2E_AUTH_IDS.EMAIL}
                                text="Email"
                                type="email"
                                placeholder={PLACEHOLDER.EMAIL}
                                onChange={changeEmail}
                                onKeyPress={onKeyPress}
                                value={email}
                                valid={checkEmail(email)}
                            />
                        </div>
                    </div>
                    <div data-testid={AUTH_IDS.BUTTON}>
                        <button
                            className={styles.purpleButton}
                            style={{
                                opacity: checkEmail(email) ? 1.0 : 0.6,
                            }}
                            onClick={forgot}
                            disabled={!checkEmail(email)}
                        >
                            Reset
                        </button>
                    </div>
                    <div className={styles.line} />
                    <div data-testid={AUTH_IDS.SWITCH}>
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
            </div>
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

export default connect(mapStateToProps)(Forgot)
