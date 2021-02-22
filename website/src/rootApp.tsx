import React, { useEffect, Dispatch } from "react"
import { connect } from "react-redux"
import { Route, Switch } from "react-router-dom"

// import Landing from "pages/homepage/landing/landing"
import About from "pages/homepage/about/about"
import TermsOfService from "pages/legal/tos"
import Cookies from "pages/legal/cookies"
import Privacy from "pages/legal/privacy"
import Auth from "pages/auth/auth"
import Verify from "pages/auth/verify/verify"
import Reset from "pages/auth/reset/reset"
import Dashboard from "pages/dashboard/dashboard"
import AuthCallback from "pages/auth/callback/callback"
import Products from "pages/homepage/products/products"

import routes from "shared/constants/routes"
import withTracker from "shared/utils/withTracker"

import * as SharedAction from "store/actions/shared"

const RootApp = (props: { dispatch: Dispatch<any> }) => {
    const { dispatch } = props
    const refreshState = () => {
        // this will do a smart merge that will basically
        // keep them logged in if they existed and update the other state variables
        // if they were updated (without nulling out stuff that is necessary)
        // and will otherwise reset to DEFAULT
        dispatch(SharedAction.refreshState())
    }

    // will refresh the state on component mount in case of
    // dev updates or other shenanigans
    useEffect(() => {
        refreshState()
        // eslint-disable-next-line react-hooks/exhaustive-deps
    }, [])

    return (
        <div>
            <Switch>
                <Route
                    exact
                    path={routes.ABOUT}
                    component={withTracker(About)}
                />
                <Route
                    exact
                    path={routes.COOKIES}
                    component={withTracker(Cookies)}
                />
                <Route
                    exact
                    path={routes.PRIVACY}
                    component={withTracker(Privacy)}
                />
                <Route
                    exact
                    path={routes.TOS}
                    component={withTracker(TermsOfService)}
                />
                <Route path={routes.AUTH} component={withTracker(Auth)} />
                <Route
                    exact
                    path={routes.VERIFY}
                    component={withTracker(Verify)}
                />
                <Route
                    exact
                    path={routes.RESET}
                    component={withTracker(Reset)}
                />
                <Route
                    exact
                    path={routes.AUTH_CALLBACK}
                    component={withTracker(AuthCallback)}
                />
                <Route
                    path={routes.DASHBOARD}
                    component={withTracker(Dashboard)}
                />
                {/* <Route
                    exact
                    path={routes.PRODUCTS}
                    component={withTracker(Products)}
                /> */}
                <Route
                    exact
                    path={routes.LANDING}
                    component={withTracker(Products)}
                />
            </Switch>
        </div>
    )
}

const mapStateToProps = () => {
    return {}
}

export default connect(mapStateToProps)(RootApp)
