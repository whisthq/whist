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
    loginEnabled,
    checkEmail,
    checkPassword,
} from "pages/auth/constants/authHelpers"
import SwitchMode from "pages/auth/components/switchMode"
import { AUTH_IDS, E2E_AUTH_IDS } from "testing/utils/testIDs"
import FractalKey from "shared/types/input"
import { User, AuthFlow } from "shared/types/reducers"

import PLACEHOLDER from "shared/constants/form"

// import GoogleButton from "pages/auth/components/googleButton"

const Login = (props: {
    dispatch: Dispatch<any>
    user: User
    authFlow: AuthFlow
}) => {
    /*
        Component for logging into the Fractal system.

        Contains the form to login, and also dispatches an API request to
        the server to authenticate the user.

        Arguments:
            dispatch (Dispatch<any>): Action dispatcher
            user (User): User from Redux state
            authFlow (AuthFlow): AuthFlow from Redux state
    */
    const { dispatch, user, authFlow } = props

    const [email, setEmail] = useState("")
    const [password, setPassword] = useState("")
    const [processing, setProcessing] = useState(false)

    // Dispatches the login API call
    const login = () => {
        if (loginEnabled(email, password)) {
            setProcessing(true)
            dispatch(AuthSideEffect.emailLogin(email, password))
        } else {
            dispatch(
                AuthPureAction.updateAuthFlow({
                    loginWarning: "Invalid email or password. Try again.",
                })
            )
        }
    }

    // so we can display puff while server does its thing for google as well
    // const google_login = (code: string) => {
    //     setProcessing(true)
    //     dispatch(AuthSideEffect.googleLogin(code))
    // }

    // Handles ENTER key press
    const onKeyPress = (evt: KeyboardEvent) => {
        if (
            evt.key === FractalKey.ENTER &&
            email.length > 4 &&
            password.length > 6 &&
            email.includes("@")
        ) {
            setProcessing(true)
            dispatch(AuthSideEffect.emailLogin(email, password))
        }
    }

    // Updates email and password fields as user types
    const changeEmail = (evt: ChangeEvent<HTMLInputElement>) => {
        evt.persist()
        setEmail(evt.target.value)
    }

    const changePassword = (evt: ChangeEvent<HTMLInputElement>) => {
        evt.persist()
        setPassword(evt.target.value)
    }

    useEffect(() => {
        if (email === "" && password === "") {
            dispatch(
                AuthPureAction.updateAuthFlow({
                    loginWarning: "",
                })
            )
        }
    }, [email, password, dispatch])

    // should trigger when they successfully log in... be it with google or with email
    useEffect(() => {
        setProcessing(false)
    }, [dispatch, user.userID, authFlow])

    if (processing) {
        // Conditionally render the loading screen as we wait for signup API call to return
        return (
            <div data-testid={AUTH_IDS.LOADING}>
                <PuffAnimation />
            </div>
        )
    } else {
        // Render the login screen
        return (
            <div>
                <div className={styles.authContainer}>
                    <div className={styles.authTitle}>Welcome back!</div>
                    {authFlow.loginWarning && authFlow.loginWarning !== "" && (
                        <div data-testid={AUTH_IDS.WARN}>
                            <div
                                className="font-body text-center"
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
                                {authFlow.loginWarning}
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
                                valid={checkEmail(email)}
                            />
                        </div>
                        <div style={{ marginTop: 13 }}>
                            <Input
                                id={E2E_AUTH_IDS.PASSWORD}
                                text="Password"
                                altText={
                                    <SwitchMode
                                        id={E2E_AUTH_IDS.FORGOTSWITCH}
                                        question=""
                                        link={PLACEHOLDER.FORGOTSWITCH}
                                        closer=""
                                        onClick={() =>
                                            dispatch(
                                                AuthPureAction.updateAuthFlow({
                                                    mode: "Forgot",
                                                })
                                            )
                                        }
                                    />
                                }
                                type="password"
                                placeholder={PLACEHOLDER.PASSWORD}
                                onChange={changePassword}
                                onKeyPress={onKeyPress}
                                value={password}
                                valid={checkPassword(password)}
                            />
                        </div>
                    </div>
                    <div data-testid={AUTH_IDS.BUTTON}>
                        <button className={styles.purpleButton} onClick={login}>
                            Log In
                        </button>
                    </div>
                    <div className={styles.line} />
                    {/* <div data-testid={}><GoogleButton login={google_login} /></div>*/}
                    <div data-testid={AUTH_IDS.SWITCH}>
                        <div style={{ marginTop: 20 }}>
                            <SwitchMode
                                question="Need to create an account?"
                                link={PLACEHOLDER.SIGNUPSWITCH}
                                closer="."
                                onClick={() =>
                                    dispatch(
                                        AuthPureAction.updateAuthFlow({
                                            mode: "Sign up",
                                        })
                                    )
                                }
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
        user: User
        authFlow: AuthFlow
    }
}) => {
    return {
        user: state.AuthReducer.user,
        authFlow: state.AuthReducer.authFlow,
    }
}

export default connect(mapStateToProps)(Login)
