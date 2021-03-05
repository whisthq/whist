import React, { useState, useEffect } from "react"
import { Row, Col } from "react-bootstrap"
import { connect } from "react-redux"
import { Redirect } from "react-router"
import { useSubscription } from "@apollo/client"
import { Switch, Route } from "react-router-dom"

import Header from "shared/components/header"
import Home from "pages/dashboard/home/home"
import Settings from "pages/dashboard/settings/settings"
import Restricted from "pages/dashboard/components/restricted/restricted"
import withTracker from "shared/utils/withTracker"
import LeftButton from "pages/dashboard/components/leftButton"
import SignoutButton from "pages/dashboard/components/signoutButton"
import { PuffAnimation } from "shared/components/loadingAnimations"
import { LeftButtonType } from "shared/types/ui"

import { HEADER, DASHBOARD_IDS, E2E_DASHBOARD_IDS } from "testing/utils/testIDs"
import { SUBSCRIBE_INVITE } from "shared/constants/graphql"

//import { CopyToClipboard } from "react-copy-to-clipboard"

import sharedStyles from "styles/shared.module.css"

import { User, AuthFlow } from "store/reducers/auth/default"

const Dashboard = (props: { user: User; location: { pathname: string } }) => {
    const { user, location } = props
    const [isRestricted, setIsRestricted] = useState(true)
    const [formFilledOut, setFormFilledOut] = useState(false)

    const { loading, error, data } = useSubscription(SUBSCRIBE_INVITE, {
        variables: {
            userID: user.userID,
        },
    })

    useEffect(() => {
        if (data && data.invites && data.invites.length > 0) {
            setIsRestricted(!data.invites[0].access_granted)
            setFormFilledOut(data.invites[0].typeform_submitted)
        } else {
            setIsRestricted(true)
            setFormFilledOut(false)
        }
    }, [loading, error, data])

    const validUser = user.userID && user.userID !== "" && user.emailVerified
    const restricted = location.pathname === "/dashboard/restricted"

    if (!validUser) {
        return <Redirect to="/auth" />
    } else if (loading) {
        return (
            <div>
                <div className="hidden md:inline relative top-36">
                    <SignoutButton id={E2E_DASHBOARD_IDS.SIGNOUT} />
                </div>
                <div data-testid={DASHBOARD_IDS.RIGHT}>
                    <PuffAnimation fullScreen/>
                </div>
            </div>
        )
    } else if (isRestricted) {
        return (
            <div className={sharedStyles.fractalContainer}>
                <div data-testid={HEADER}>
                    <Header account />
                </div>
                <div className="hidden md:inline relative top-36">
                    <SignoutButton id={E2E_DASHBOARD_IDS.SIGNOUT} />
                </div>
                <div data-testid={DASHBOARD_IDS.RIGHT}>
                    <Restricted formFilledOut={formFilledOut} />
                </div>
            </div>
        )
    } else {
        return (
            <div className={sharedStyles.fractalContainer}>
                <div data-testid={HEADER}>
                    <Header account />
                </div>
                <Row>
                    {!restricted && (
                        <Col md={1} className="hidden md:inline mt-44">
                            <LeftButton
                                id={E2E_DASHBOARD_IDS.HOME}
                                exact={true}
                                path="/dashboard"
                                iconName={LeftButtonType.HOME}
                            />
                            <LeftButton
                                id={E2E_DASHBOARD_IDS.SETTINGS}
                                exact={false}
                                path="/dashboard/settings"
                                iconName={LeftButtonType.SETTINGS}
                            />
                            <SignoutButton id={E2E_DASHBOARD_IDS.SIGNOUT} />
                        </Col>
                    )}
                    <Col sm={12} md={restricted ? 12 : 11} className="mt-12">
                        <div data-testid={DASHBOARD_IDS.RIGHT}>
                            <Switch>
                                <Route
                                    exact
                                    path="/dashboard"
                                    component={withTracker(Home)}
                                />
                                <Route
                                    path="/dashboard/settings"
                                    component={withTracker(Settings)}
                                />
                            </Switch>
                        </div>
                    </Col>
                </Row>
            </div>
        )
    }
}

const mapStateToProps = (state: {
    AuthReducer: { user: User; authFlow: AuthFlow }
}) => {
    return {
        user: state.AuthReducer.user,
    }
}

export default connect(mapStateToProps)(Dashboard)
