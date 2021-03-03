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
import PasswordConfirmForm from "shared/components/passwordConfirmForm"

// Function imports
import {emailSignup} from "store/actions/auth/sideEffects"
import {updateAuthFlow} from "store/actions/auth/pure"
import {
    checkPasswordVerbose,
    checkEmailVerbose,
    signupEnabled,
    checkEmail,
} from "pages/auth/constants/authHelpers"
import {routeMap, fractalRoute} from "shared/constants/routes"

// Type + constant imports
import FractalKey from "shared/types/input"
import { User, AuthFlow } from "shared/types/reducers"
import PLACEHOLDER from "shared/constants/form"
import { AUTH_IDS, E2E_AUTH_IDS } from "testing/utils/testIDs"

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
            dispatch(emailSignup(email, name, password))
        } else {
            dispatch(updateAuthFlow({signupWarning: "Please fill out all fields"}))
        }
    }

    // Handles ENTER key press
    const onKeyPress = (evt: KeyboardEvent) => {
        if (
            evt.key === FractalKey.ENTER
        ) {
            signup()
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

    // For warnings
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

    // Render the UI
    if (processing) {
        return (
            <div data-testid={AUTH_IDS.LOADING}>
                <PuffAnimation />
            </div>
        )
    } else {
        return (
            <AuthContainer title="Sign up for an account">
                <AuthWarning warning={authFlow.signupWarning}/>
                <div data-testid={AUTH_IDS.FORM}>
                    <div className="mt-7">
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
                    <div className="mt-3">
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
                    <AuthButton text="Sign up" onClick={signup} dataTestId={AUTH_IDS.BUTTON} />
                    <div data-testid={AUTH_IDS.SWITCH}>
                        <div className="mt-7">
                            <AuthNavigator
                                id={E2E_AUTH_IDS.LOGINSWITCH}
                                beforeLink="Already have an account?"
                                link={PLACEHOLDER.LOGINSWITCH}
                                afterLink="."
                                redirect={fractalRoute(routeMap.AUTH)}
                            />
                        </div>
                    </div>
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

export default connect(mapStateToProps)(Signup)
