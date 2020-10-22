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

import * as SharedAction from "store/actions/shared"

const RootApp = (props: any) => {
    const { dispatch, user } = props

    const refreshState = () => {
        if (!user.user_id) {
            dispatch(SharedAction.resetState())
        }
    }

    useEffect(() => {
        refreshState()
    }, [])

    return (
        <div>
            <Switch>
                <Route exact path="/about" component={About} />
                <Route exact path="/application" component={Application} />
                <Route exact path="/cookies" component={Cookies} />
                <Route exact path="/privacy" component={Privacy} />
                <Route
                    exact
                    path="/termsofservice"
                    component={TermsOfService}
                />
                <Route exact path="/auth" component={Auth} />
                <Route exact path="/verify" component={Verify} />
                <Route exact path="/reset" component={Reset} />
                <Route exact path="/dashboard" component={Dashboard} />
                <Route exact path="/:first?/:second?" component={Landing} />
            </Switch>
        </div>
    )
}

function mapStateToProps(state: { AuthReducer: { user: any } }) {
    return {
        user: state.AuthReducer.user,
    }
}

export default connect(mapStateToProps)(RootApp)
