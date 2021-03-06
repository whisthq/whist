import React, { useEffect } from "react"
import { connect } from "react-redux"
import { Route, Switch } from "react-router-dom"

import Landing from "pages/landing/landing"
import Application from "pages/application/application"
import About from "pages/about/about"
import TermsOfService from "pages/legal/tos"
import Cookies from "pages/legal/cookies"
import Privacy from "pages/legal/privacy"
import Auth from "pages/auth/auth"
import Verify from "pages/auth/verify"
import Reset from "pages/auth/reset"
import Dashboard from "pages/dashboard/dashboard"
import Profile from "pages/profile/profile"
import AuthCallback from "pages/auth/callback"
import Plan from "pages/plan/plan"
import Payment from "pages/payment/payment"
import Cancel from "pages/cancel/cancel"
import Canceled from "pages/cancel/canceled"
import Confirmation from "pages/confirmation/confirmation"

import withTracker from "shared/utils/withTracker"

import * as SharedAction from "store/actions/shared"

const RootApp = (props: any) => {
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
                <Route exact path="/about" component={withTracker(About)} />
                <Route
                    exact
                    path="/application"
                    component={withTracker(Application)}
                />
                <Route exact path="/cookies" component={withTracker(Cookies)} />
                <Route exact path="/privacy" component={withTracker(Privacy)} />
                <Route
                    exact
                    path="/termsofservice"
                    component={withTracker(TermsOfService)}
                />
                <Route
                    path="/auth/:first?/:second?"
                    component={withTracker(Auth)}
                />
                <Route exact path="/verify" component={withTracker(Verify)} />
                <Route exact path="/reset" component={withTracker(Reset)} />
                <Route
                    exact
                    path="/callback"
                    component={withTracker(AuthCallback)}
                />
                <Route
                    exact
                    path="/dashboard"
                    component={withTracker(Dashboard)}
                />
                <Route exact path="/profile" component={withTracker(Profile)} />
                <Route exact path="/plan" component={withTracker(Plan)} />
                <Route exact path="/payment" component={withTracker(Payment)} />
                <Route
                    exact
                    path="/confirmation"
                    component={withTracker(Confirmation)}
                />
                <Route exact path="/cancel" component={withTracker(Cancel)} />
                <Route
                    exact
                    path="/canceled"
                    component={withTracker(Canceled)}
                />
                <Route
                    exact
                    path="/:first?/:second?"
                    component={withTracker(Landing)}
                />
            </Switch>
        </div>
    )
}

function mapStateToProps(state: any) {
    return {}
}

export default connect(mapStateToProps)(RootApp)
