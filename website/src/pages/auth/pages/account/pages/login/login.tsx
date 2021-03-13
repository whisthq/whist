// npm imports
import React, {
    useState,
    ChangeEvent,
    KeyboardEvent,
    Dispatch,
} from "react"
import { connect } from "react-redux"

// Component imports
import AuthContainer from "pages/auth/shared/components/authContainer"
import AuthWarning from "pages/auth/shared/components/authWarning"
import AuthButton from "pages/auth/shared/components/authButton"
import AuthNavigator from "pages/auth/shared/components/authNavigator"
import Input from "shared/components/input"

// Function imports
import {emailLogin} from "shared/api/index"
import {
    loginEnabled,
    checkEmail,
    checkPassword,
} from "pages/auth/shared/helpers/authHelpers"
import {routeMap, fractalRoute} from "shared/constants/routes"
import {updateUser} from "store/actions/auth/pure"

// Type + constant imports
import FractalKey from "shared/types/input"
import PLACEHOLDER from "shared/constants/form"
import { AUTH_IDS, E2E_AUTH_IDS } from "testing/utils/testIDs"

const Login = (props: {
    dispatch: Dispatch<any>
}) => {
    /*
        Component for logging into the Fractal system.

        Contains the form to login, and also dispatches an API request to
        the server to authenticate the user.

        Arguments:
            dispatch (Dispatch<any>): Action dispatcher
            user (User): User from Redux state
    */
    const { dispatch } = props

    const [email, setEmail] = useState("")
    const [password, setPassword] = useState("")
    const [processing, setProcessing] = useState(false)
    const [loginWarning, setLoginWarning] = useState("")

    // Sends login API request
    const login = () => {
        if (loginEnabled(email, password)) {
            setProcessing(true)
            emailLogin(email, password).then(({json}) => {
                if(json && json.access_token) {
                    dispatch(updateUser({
                        userID: email,
                        name: json.name,
                        accessToken: json.access_token,
                        refreshToken: json.refresh_token,
                        emailVerified: json.verified,
                        emailVerificationToken: json.verification_token
                    }))
                } else {
                    // TODO: Failure logic
                }
            })
        } else {
            setLoginWarning("Invalid email or password. Try again.")
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

    return (
        <AuthContainer title="Log in to your account">
            <AuthWarning warning={loginWarning}/>
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
                            redirect={fractalRoute(routeMap.AUTH.FORGOT)}
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
            <AuthButton text="Log in" onClick={login} dataTestId={AUTH_IDS.BUTTON} processing={processing}/>
            <div className="mt-7" data-testid={AUTH_IDS.SWITCH}>
                <AuthNavigator
                    beforeLink="No account?"
                    link={PLACEHOLDER.SIGNUPSWITCH}
                    afterLink="."
                    redirect={fractalRoute(routeMap.AUTH.ACCOUNT.SIGNUP)}
                />
            </div>
        </AuthContainer>
    )
}


export default connect()(Login)
