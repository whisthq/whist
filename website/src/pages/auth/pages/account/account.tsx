// npm imports
import React from "react"
import { Switch, Route } from "react-router-dom"

// Component imports
import Login from "pages/auth/pages/account/pages/login/login"
import Signup from "pages/auth/pages/account/pages/signup/signup"
import Verify from "pages/auth/pages/account/pages/verify/verify"

// Function imports
import { routeMap, fractalRoute } from "shared/constants/routes"

const Account = () => {
    return (
        <>
            <Switch>
                <Route
                    exact
                    path={[fractalRoute(routeMap.AUTH.ACCOUNT), fractalRoute(routeMap.AUTH)]}
                    component={Login}
                />
                <Route
                    path={fractalRoute(routeMap.AUTH.ACCOUNT.LOGIN)}
                    component={Login}
                />
                <Route
                    path={fractalRoute(routeMap.AUTH.ACCOUNT.SIGNUP)}
                    component={Signup}
                />
                <Route
                    path={fractalRoute(routeMap.AUTH.ACCOUNT.VERIFY)}
                    component={Verify}
                />
            </Switch>
        </>
    )
}

export default Account