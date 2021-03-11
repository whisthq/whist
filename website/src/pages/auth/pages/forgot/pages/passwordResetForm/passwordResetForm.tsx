// npm imports
import React, { useEffect } from "react"
import { useLocation } from "react-router-dom"
import { Switch, Route } from "react-router-dom"

// Function imports
import { validateAccessToken } from "shared/api/index"
import { routeMap, fractalRoute } from "shared/constants/routes"
import { useFractalProvider } from "pages/auth/pages/forgot/shared/store/store"
import history from "shared/utils/history"

// Component imports
import AuthLoader from "pages/auth/shared/components/authLoader"
import Allowed from "pages/auth/pages/forgot/pages/passwordResetForm/pages/allowed/allowed"
import Success from "pages/auth/pages/forgot/pages/passwordResetForm/pages/success/success"
import Error from "pages/auth/pages/forgot/pages/passwordResetForm/pages/error/error"

// Constant + type imports
import { FractalHTTPCode } from "@fractal/core-ts/types/api"

const PasswordResetForm = () => {
    const { dispatch } = useFractalProvider()
    // Read the access token from the URL, which determines if the user is allowed to password reset
    const accessToken = useLocation().search.substring(1, useLocation().search.length)
    const loadingTitle = "Please wait while we authenticate you"

    // On page load, read the access token from the URL and validate it to determine
    // if the user is allowed to reset their password
    useEffect(() => {
        if (accessToken) {
            validateAccessToken(accessToken).then(({json}) => {
                if(json && json.status === FractalHTTPCode.SUCCESS) {
                    dispatch({body: {
                        accessToken: accessToken
                    }})
                    history.push(fractalRoute(routeMap.AUTH.FORGOT.RESET.ALLOWED))
                } else {
                    history.push(fractalRoute(routeMap.AUTH.FORGOT.RESET.ERROR))
                }
            })
        } else {
            history.push(fractalRoute(routeMap.AUTH.FORGOT.RESET.ERROR))
        }
    }, [])

    return (
        <>
            <Switch>
                <Route
                    exact
                    path={fractalRoute(routeMap.AUTH.FORGOT.RESET)}
                    render={() => <AuthLoader title={loadingTitle} />}
                />
                <Route
                    path={fractalRoute(routeMap.AUTH.FORGOT.RESET.ALLOWED)}
                    component={Allowed}
                />
                <Route
                    path={fractalRoute(routeMap.AUTH.FORGOT.RESET.SUCCESS)}
                    component={Success}
                />
                <Route
                    path={fractalRoute(routeMap.AUTH.FORGOT.RESET.ERROR)}
                    component={Error}
                />
            </Switch>
        </>
    )
}

export default PasswordResetForm
