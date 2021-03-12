import React, { useEffect, useState, Dispatch } from "react"
import { connect } from "react-redux"
import { useMutation } from "@apollo/client"
import { Switch, Route } from "react-router-dom"

import Login from "pages/auth/pages/account/login/login"
import Signup from "pages/auth/pages/account/signup/signup"
import Forgot from "pages/auth/pages/forgot/forgot"
import Verify from "pages/auth/pages/account/verify/verify"
import Callback from "pages/auth/pages/callback/callback"
import ScrollToTop from "shared/components/scrollToTop"

import { User, AuthFlow } from "store/reducers/auth/default"
import { updateAuthFlow } from "store/actions/auth/pure"
import { UPDATE_ACCESS_TOKEN } from "shared/constants/graphql"
import { routeMap, fractalRoute } from "shared/constants/routes"

const Auth = (props: {
    /*
        Wrapper component for all authentication components (sign in,
        client-app redirect)
 
        Quick summary of the authentication flow here: 
            https://www.notion.so/tryfractal/How-does-our-auth-flow-work-94ae6ae18742491795bb2f565fedc827

        Arguments:
            dispatch (Dispatch<any>): Action dispatcher
            user (User): User from Redux state
            authFlow (AuthFlow): AuthFlow from Redux state
            location: a Location object from react router
            testSignup: a test boolean to determine whether to test signup or not
            emailToken (String): email verification token

    */
    dispatch: Dispatch<any>
    user: User
    authFlow: AuthFlow
    location: {
        pathname: string
    }
}) => {
    const { user, authFlow, location, dispatch } = props

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
    }, [dispatch, props.location.pathname, props.location])

    // If a callback was provided, save it to Redux
    useEffect(() => {
        if (callback && callback !== "") {
            dispatch(updateAuthFlow({ callback: callback }))
        }
    }, [callback, dispatch])

    // If Redux callback found and we have not redirected to the callback yet, then redirect
    useEffect(() => {
        if (
            !redirectToCallback &&
            authFlow.callback &&
            authFlow.callback !== "" &&
            callbackChecked &&
            user.accessToken &&
            user.emailVerified
        ) {
            updateAccessToken({
                variables: {
                    loginToken: loginToken,
                    accessToken: user.accessToken,
                },
            }).catch((error) => {
                throw error
            })

            window.location.replace(authFlow.callback)
            setRedirectToCallback(true)
        }
    }, [
        authFlow.callback,
        redirectToCallback,
        user,
        user.accessToken,
        user.emailVerified,
        loginToken,
        updateAccessToken,
        callbackChecked,
    ])

    return(
        <div>
            <ScrollToTop/>
            <Switch>
                <Route exact path={fractalRoute(routeMap.AUTH)} component={Login}/>
                <Route path={fractalRoute(routeMap.AUTH.SIGNUP)} component={Signup}/>
                <Route path={[fractalRoute(routeMap.AUTH.FORGOT.EMAIL), fractalRoute(routeMap.AUTH.FORGOT.RESET)]} component={Forgot}/>
                <Route path={fractalRoute(routeMap.AUTH.VERIFY)} component={Verify}/>
                <Route path={fractalRoute(routeMap.AUTH.CALLBACK)} component={Callback}/>
            </Switch>
        </div>
    )
}

const mapStateToProps = (state: {
    AuthReducer: { authFlow: AuthFlow; user: User }
}) => {
    return {
        user: state.AuthReducer.user,
        authFlow: state.AuthReducer.authFlow,
    }
}

export default connect(mapStateToProps)(Auth)
