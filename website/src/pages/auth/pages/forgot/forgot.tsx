// npm imports
import React from "react"
import { Switch, Route } from "react-router-dom"

// Component imports
import EmailForm from "pages/auth/pages/forgot/pages/emailForm/emailForm"
import PasswordResetForm from "pages/auth/pages/forgot/pages/passwordResetForm/passwordResetForm"
import { FractalProvider } from "pages/auth/pages/forgot/shared/store/store"

// Constant + type imports
import { routeMap, fractalRoute } from "shared/constants/routes"

const Forgot = () => {
    /*
        Component for when the user forgets their login information.
    */
    return (
        <>
            <FractalProvider>
                <Switch>
                    <Route
                        path={fractalRoute(routeMap.AUTH.FORGOT.EMAIL)}
                        component={EmailForm}
                    />
                    <Route
                        path={fractalRoute(routeMap.AUTH.FORGOT.RESET)}
                        component={PasswordResetForm}
                    />
                </Switch>
            </FractalProvider>
        </>
    )
}

export default Forgot
