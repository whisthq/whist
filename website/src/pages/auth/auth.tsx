import React, { useEffect, useState, Dispatch } from "react"
import { connect } from "react-redux"
import { Redirect } from "react-router"
import { useMutation } from "@apollo/client"

import Header from "shared/components/header"
import Login from "pages/auth/login/login"
import Signup from "pages/auth/signup/signup"
import Forgot from "pages/auth/forgot/forgot"

import sharedStyles from "styles/shared.module.css"

import { User, AuthFlow } from "shared/types/reducers"
import { updateAuthFlow } from "store/actions/auth/pure"
import { HEADER, AUTH_IDS, NEVER } from "testing/utils/testIDs"
import { UPDATE_ACCESS_TOKEN } from "shared/constants/graphql"

const Auth = (props: {
    /*
        Wrapper component for all authentication components (sign in,
        client-app redirect)
 
        Quick summary of the authentication flow here: https://www.notion.so/tryfractal/How-does-our-auth-flow-work-94ae6ae18742491795bb2f565fedc827

        Arguments:
            dispatch (Dispatch<any>): Action dispatcher
            user (User): User from Redux state
            authFlow (AuthFlow): AuthFlow from Redux state
            location: a Location object from react router
            testLocation: a test Location object for testing this component
            testSignup: a test boolean to determine whether to test signup or not
            emailToken (String): email verification token

    */
    dispatch: Dispatch<any>
    user: User
    mode: string
    authFlow: AuthFlow
    location: {
        pathname: string
    }
    testLocation?: any
    testSignup?: boolean
    emailToken?: string
}) => {
    const { user, mode, authFlow, testSignup, emailToken, dispatch } = props

    const [redirectToCallback, setRedirectToCallback] = useState(false)
    const [callback, setCallback] = useState("")
    const [callbackChecked, setCallbackChecked] = useState(false)
    const [loginToken, setLoginToken] = useState("")

    const [updateAccessToken] = useMutation(UPDATE_ACCESS_TOKEN, {
        context: {
            headers: {
                "X-Hasura-Login-Token": loginToken,
            },
        },
    })

    useEffect(() => {
        let location = props.testLocation ? props.testLocation : props.location

        // Clear any previous callback URL
        dispatch(
            updateAuthFlow({
                callback: undefined,
                loginWarning: "",
                signupWarning: "",
            })
        )

        // Check to see if URL contains a login token
        if (
            location.pathname &&
            location.pathname.substring(6, 17) === "loginToken="
        ) {
            setLoginToken(location.pathname.substring(17))
            setCallback("fractal://")
        }

        setCallbackChecked(true)
    }, [dispatch, props.location.pathname, props.testLocation, props.location])

    // If a callback was provided, save it to Redux
    useEffect(() => {
        if (callback && callback !== "") {
            dispatch(updateAuthFlow({ callback: callback }))
        }
    }, [callback, dispatch, authFlow.callback, user.accessToken])

    // If Redux callback found and we have not redirected to the callback yet, then redirect
    useEffect(() => {
        if (
            !redirectToCallback &&
            authFlow.callback &&
            authFlow.callback !== "" &&
            callbackChecked &&
            user.accessToken &&
            user.emailVerified &&
            user.configToken
        ) {
            updateAccessToken({
                variables: {
                    loginToken: loginToken,
                    accessToken: user.accessToken,
                },
            }).catch((error) => {
                throw error
            })

            if (!props.testLocation) window.location.replace(authFlow.callback)

            setRedirectToCallback(true)
        }
    }, [
        authFlow.callback,
        props.testLocation,
        redirectToCallback,
        user,
        user.accessToken,
        user.emailVerified,
        user.configToken,
        loginToken,
        updateAccessToken,
        callbackChecked,
    ])

    if (redirectToCallback) {
        return (
            <Redirect to="/callback" />
        )
    }

    if (user.userID && user.userID !== "") {
        if (user.emailVerified && callback === "" && callbackChecked) {
            return <Redirect to="/dashboard" />
        } else if (!user.emailVerified && callback === "" && callbackChecked) {
            // testing email verification token just to get values to check user flow
            if (testSignup === true || testSignup === false) {
                return (
                    <div>
                        <p data-testid={AUTH_IDS.EMAILTOKEN}>
                            {user.emailVerificationToken}
                        </p>
                        <p data-testid={AUTH_IDS.USERID}>{user.userID}</p>
                        <p data-testid={AUTH_IDS.REFRESHTOKEN}>
                            {user.refreshToken}
                        </p>
                        <p data-testid={AUTH_IDS.ACCESSTOKEN}>
                            {user.accessToken}
                        </p>
                    </div>
                )
            }
            return <Redirect to="/verify" />
        }
    }

    if (mode === "Log in") {
        return (
            <div className={sharedStyles.fractalContainer}>
                <div data-testid={HEADER}>
                    <Header dark={false} />
                </div>
                <div data-testid={AUTH_IDS.LOGIN}>
                    <Login />
                </div>
            </div>
        )
    } else if (mode === "Sign up") {
        return (
            <div className={sharedStyles.fractalContainer}>
                <div data-testid={HEADER}>
                    <Header dark={false} />
                </div>
                <div data-testid={AUTH_IDS.SIGNUP}>
                    <Signup />
                </div>
            </div>
        )
    } else if (mode === "Forgot") {
        return (
            <div className={sharedStyles.fractalContainer}>
                <div data-testid={HEADER}>
                    <Header dark={false} />
                </div>
                <div data-testid={AUTH_IDS.FORGOT}>
                    <Forgot emailToken={emailToken} />
                </div>
            </div>
        )
    } else {
        // should never happen
        return <div data-testid={NEVER} />
    }
}

const mapStateToProps = (state: {
    AuthReducer: { authFlow: AuthFlow; user: User }
}) => {
    return {
        mode:
            state.AuthReducer.authFlow && state.AuthReducer.authFlow.mode
                ? state.AuthReducer.authFlow.mode
                : "Sign up",
        user: state.AuthReducer.user,
        authFlow: state.AuthReducer.authFlow,
    }
}

export default connect(mapStateToProps)(Auth)
