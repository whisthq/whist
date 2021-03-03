// npm imports
import React, {
    useState,
    useEffect,
    ChangeEvent,
    KeyboardEvent,
    Dispatch,
} from "react"
import { connect } from "react-redux"

// Component imports
import AuthContainer from "pages/auth/components/authContainer"
import AuthWarning from "pages/auth/components/authWarning"
import AuthButton from "pages/auth/components/authButton"
import AuthNavigator from "pages/auth/components/authNavigator"
import { PuffAnimation } from "shared/components/loadingAnimations"
import Input from "shared/components/input"

// Function imports
import {emailLogin} from "store/actions/auth/sideEffects"
import {updateAuthFlow} from "store/actions/auth/pure"
import {
    loginEnabled,
    checkEmail,
    checkPassword,
} from "pages/auth/constants/authHelpers"

// Type + constant imports
import FractalKey from "shared/types/input"
import { User, AuthFlow } from "shared/types/reducers"
import PLACEHOLDER from "shared/constants/form"
import { AUTH_IDS, E2E_AUTH_IDS } from "testing/utils/testIDs"

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

    // Sends login API request
    const login = () => {
        if (loginEnabled(email, password)) {
            setProcessing(true)
            dispatch(emailLogin(email, password))
        } else {
            dispatch(
                updateAuthFlow({
                    loginWarning: "Invalid email or password. Try again.",
                })
            )
        }
    }

    // Handles the ENTER key
    const onKeyPress = (evt: KeyboardEvent) => {
        if (
            evt.key === FractalKey.ENTER &&
            email.length > 4 &&
            password.length > 6 &&
            email.includes("@")
        ) {
            setProcessing(true)
            dispatch(emailLogin(email, password))
        }
    }

    // Handles email form keystrokes
    const changeEmail = (evt: ChangeEvent<HTMLInputElement>) => {
        evt.persist()
        setEmail(evt.target.value)
    }

    // Handles password form keystrokes
    const changePassword = (evt: ChangeEvent<HTMLInputElement>) => {
        evt.persist()
        setPassword(evt.target.value)
    }

    // If the form is blank, hide warnings
    useEffect(() => {
        if (email === "" && password === "") {
            dispatch(
                updateAuthFlow({
                    loginWarning: "",
                })
            )
        }
    }, [email, password, dispatch])

    // When the User or Authflow state changes, stop the spinning wheel
    useEffect(() => {
        setProcessing(false)
    }, [user.userID, authFlow])

    // Render the UI
    if (processing) {
        return (
            <div data-testid={AUTH_IDS.LOADING}>
                <PuffAnimation />
            </div>
        )
    } else {
        return (
            <AuthContainer title="Log in to your account">
                <AuthWarning warning={authFlow.loginWarning}/>
                <div className="mt-7" data-testid={AUTH_IDS.FORM}>
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
                <div className="mt-4">
                    <Input
                        id={E2E_AUTH_IDS.PASSWORD}
                        text="Password"
                        altText={
                            <AuthNavigator
                                id={E2E_AUTH_IDS.FORGOTSWITCH}
                                link={PLACEHOLDER.FORGOTSWITCH}
                                onClick={() =>
                                    dispatch(
                                        updateAuthFlow({
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
                <AuthButton text="Log in" onClick={login} dataTestId={AUTH_IDS.BUTTON}/>
                <div className="mt-7" data-testid={AUTH_IDS.SWITCH}>
                    <AuthNavigator
                        question="No account?"
                        link={PLACEHOLDER.SIGNUPSWITCH}
                        closer="."
                        onClick={() =>
                            dispatch(
                                updateAuthFlow({
                                    mode: "Sign up",
                                })
                            )
                        }
                    />
                </div>
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
        user: state.AuthReducer.user,
        authFlow: state.AuthReducer.authFlow,
    }
}

export default connect(mapStateToProps)(Login)
