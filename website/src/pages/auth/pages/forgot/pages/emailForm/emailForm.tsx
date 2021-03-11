// npm import
import React from "react"
import { Switch, Route } from "react-router-dom"

// Component imports
import Form from "pages/auth/pages/forgot/pages/emailForm/pages/form/form"
import FormSubmitted from "pages/auth/pages/forgot/pages/emailForm/pages/formSubmitted/formSubmitted"

// Function imports
import { routeMap, fractalRoute } from "shared/constants/routes"

const EmailForm = () => {
    /*
        Component for user to provide their email to receive a password
        reset email
    */
    return(
        <>
            <Switch>
                <Route
                    exact
                    path={fractalRoute(routeMap.AUTH.FORGOT)}
                    component={Form}
                />
                <Route
                    path={fractalRoute(routeMap.AUTH.FORGOT.EMAIL.SUBMITTED)}
                    component={FormSubmitted}
                />
            </Switch>
        </>
    )
}

export default EmailForm
