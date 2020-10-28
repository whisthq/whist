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

import withTracker from "shared/utils/withTracker"

// import * as SharedAction from "store/actions/shared"

const RootApp = (props: any) => {
    // const { dispatch, user } = props

    const refreshState = () => {
        // if (!user.user_id) {
        //     dispatch(SharedAction.resetState())
        // }
    }

    // temp reset state so that some bugs will be fixed basically
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
                <Route exact path="/auth" component={withTracker(Auth)} />
                <Route exact path="/verify" component={withTracker(Verify)} />
                <Route exact path="/reset" component={withTracker(Reset)} />
                {
                    <Route
                        exact
                        path="/dashboard"
                        component={withTracker(Dashboard)}
                    />
                }
                <Route
                    exact
                    path="/:first?/:second?"
                    component={withTracker(Landing)}
                />
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
