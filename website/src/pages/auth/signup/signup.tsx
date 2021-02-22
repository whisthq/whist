import React, {
    useState,
    useEffect,
    ChangeEvent,
    KeyboardEvent,
    Dispatch,
} from "react"
import { connect } from "react-redux"
import { PuffAnimation } from "shared/components/loadingAnimations"

import styles from "styles/auth.module.css"

import Input from "shared/components/input"
import * as AuthSideEffect from "store/actions/auth/sideEffects"
import * as AuthPureAction from "store/actions/auth/pure"
import {
    checkPasswordVerbose,
    checkEmailVerbose,
    signupEnabled,
    checkEmail,
} from "pages/auth/constants/authHelpers"
import SwitchMode from "pages/auth/components/switchMode"
import PasswordConfirmForm from "shared/components/passwordConfirmForm"
import { AUTH_IDS, E2E_AUTH_IDS } from "testing/utils/testIDs"
import FractalKey from "shared/types/input"
import { User, AuthFlow } from "shared/types/reducers"
import PLACEHOLDER from "shared/constants/form"

// import GoogleButton from "pages/auth/components/googleButton"

const Signup = (props: {
    dispatch: Dispatch<any>
    user: User
    authFlow: AuthFlow
}) => {
    /*
        Component for signing up for Fractal

        Contains the form to signup, and also dispatches an API request to
        the server to authenticate the user.

        Arguments:
            dispatch (Dispatch<any>): Action dispatcher
            user (User): User from Redux state
            authFlow (AuthFlow): AuthFlow from Redux state
    */
    const { authFlow, user, dispatch } = props

    const [email, setEmail] = useState("")
    const [name, setName] = useState("")
    const [password, setPassword] = useState("")
    const [confirmPassword, setConfirmPassword] = useState("")
    const [emailWarning, setEmailWarning] = useState("")
    const [nameWarning, setNameWarning] = useState("")
    const [passwordWarning, setPasswordWarning] = useState("")
    const [confirmPasswordWarning, setConfirmPasswordWarning] = useState("")
    const [enteredName, setEnteredName] = useState(false)

    const [processing, setProcessing] = useState(false)

    // Dispatches signup API call
    const signup = () => {
        if (signupEnabled(email, name, password, confirmPassword)) {
            setProcessing(true)
            dispatch(AuthSideEffect.emailSignup(email, name, password))
        }
    }

    // so we can display puff while server does it's thing for google as well
    // const google_signup = (code: string) => {
    //     setProcessing(true)
    //     dispatch(AuthSideEffect.googleLogin(code))
    // }

    // Handles ENTER key press
    const onKeyPress = (evt: KeyboardEvent) => {
        if (
            evt.key === FractalKey.ENTER &&
            email.length > 4 &&
            name.length > 0 &&
            password.length > 6 &&
            email.includes("@")
        ) {
            setProcessing(true)
            dispatch(AuthSideEffect.emailSignup(email, name, password))
        }
    }

    // Updates email, password, confirm password fields as user types
    const changeEmail = (evt: ChangeEvent<HTMLInputElement>) => {
        evt.persist()
        setEmail(evt.target.value)
    }

    const changeName = (evt: ChangeEvent<HTMLInputElement>) => {
        if (!enteredName) {
            setEnteredName(true)
        }
        evt.persist()
        setName(evt.target.value)
    }

    const changePassword = (evt: ChangeEvent<HTMLInputElement>) => {
        evt.persist()
        setPassword(evt.target.value)
    }

    const changeConfirmPassword = (evt: ChangeEvent<HTMLInputElement>) => {
        evt.persist()
        setConfirmPassword(evt.target.value)
    }

    const checkNameVerbose = (n: string): string => {
        if (n.length > 0) {
            return ""
        } else {
            return "Required"
        }
    }

    const checkName = (n: string): boolean => {
        return n.length > 0
    }

    // Removes loading screen on prop change and page load
    useEffect(() => {
        setProcessing(false)
    }, [user.userID, authFlow])

    // Email and password dynamic warnings
    useEffect(() => {
        setEmailWarning(checkEmailVerbose(email))
    }, [email])

    useEffect(() => {
        setPasswordWarning(checkPasswordVerbose(password))
    }, [password])

    useEffect(() => {
        if (enteredName) {
            setNameWarning(checkNameVerbose(name))
        } else {
            setNameWarning("")
        }
    }, [name, enteredName])

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
        // Conditionally render the loading screen as we wait for signup API call to return
        return (
            <div data-testid={AUTH_IDS.LOADING}>
                <PuffAnimation />
            </div>
        )
    } else {
        // Render the signup screen
        return (
            <div>
                <div className={styles.authContainer}>
                    <div className={styles.authTitle}>Let's get started.</div>
                    {authFlow &&
                        authFlow.signupWarning &&
                        authFlow.signupWarning !== "" && (
                            <div data-testid={AUTH_IDS.WARN}>
                                <div
                                    style={{
                                        width: "100%",
                                        background: "#ff5627",
                                        padding: "15px 20px",
                                        color: "white",
                                        fontSize: 14,
                                        marginTop: 5,
                                        marginBottom: 20,
                                    }}
                                >
                                    {authFlow.signupWarning}
                                </div>
                            </div>
                        )}
                    <div data-testid={AUTH_IDS.FORM}>
                        <div style={{ marginTop: 13 }}>
                            <Input
                                id={E2E_AUTH_IDS.EMAIL}
                                text="Email"
                                type="email"
                                placeholder={PLACEHOLDER.EMAIL}
                                onChange={changeEmail}
                                onKeyPress={onKeyPress}
                                value={email}
                                warning={emailWarning}
                                valid={checkEmail(email)}
                            />
                        </div>
                        <div style={{ marginTop: 13 }}>
                            <Input
                                id={E2E_AUTH_IDS.NAME}
                                text="Name"
                                type="name"
                                placeholder={PLACEHOLDER.NAME}
                                onChange={changeName}
                                onKeyPress={onKeyPress}
                                value={name}
                                warning={nameWarning}
                                valid={checkName(name)}
                            />
                        </div>
                        <PasswordConfirmForm
                            changePassword={changePassword}
                            changeConfirmPassword={changeConfirmPassword}
                            onKeyPress={onKeyPress}
                            password={password}
                            confirmPassword={confirmPassword}
                            passwordWarning={passwordWarning}
                            confirmPasswordWarning={confirmPasswordWarning}
                            isFirstElement={false}
                        />
                        <div data-testid={AUTH_IDS.BUTTON}>
                            <button
                                className={styles.purpleButton}
                                style={{
                                    opacity: signupEnabled(
                                        email,
                                        name,
                                        password,
                                        confirmPassword
                                    )
                                        ? 1.0
                                        : 0.6,
                                }}
                                onClick={signup}
                                disabled={
                                    !signupEnabled(
                                        email,
                                        name,
                                        password,
                                        confirmPassword
                                    )
                                }
                            >
                                Sign up
                            </button>
                        </div>
                        <div className={styles.line} />
                        {/* <GoogleButton login={google_signup} /> */}
                        <div data-testid={AUTH_IDS.SWITCH}>
                            <div style={{ marginTop: 20 }}>
                                <SwitchMode
                                    id={E2E_AUTH_IDS.LOGINSWITCH}
                                    question="Already have an account?"
                                    link={PLACEHOLDER.LOGINSWITCH}
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
            </div>
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
        user: state.AuthReducer.user,
        authFlow: state.AuthReducer.authFlow,
    }
}

export default connect(mapStateToProps)(Signup)
