import React from "react"
import { Row, Col } from "react-bootstrap"
import { connect } from "react-redux"
import { Redirect } from "react-router"
import { Switch, Route } from "react-router-dom"

import Header from "shared/components/header"
import Home from "pages/dashboard/home/home"
import Settings from "pages/dashboard/settings/settings"
import withTracker from "shared/utils/withTracker"
import LeftButton from "pages/dashboard/components/leftButton"
import SignoutButton from "pages/dashboard/components/signoutButton"

import { LeftButtonType } from "shared/types/ui"

import { HEADER, DASHBOARD_IDS, E2E_DASHBOARD_IDS } from "testing/utils/testIDs"

//import { CopyToClipboard } from "react-copy-to-clipboard"

import sharedStyles from "styles/shared.module.css"

import { User, AuthFlow } from "shared/types/reducers"

const Dashboard = (props: { user: User }) => {
    const { user } = props

    //const [copiedtoClip, setCopiedtoClip] = useState(false)
    //const linuxCommands = "sudo apt-get install libavcodec-dev libavdevice-dev libx11-dev libxtst-dev xclip x11-xserver-utils -y"
    const validUser = user.userID && user.userID !== ""

    if (!validUser) {
        return <Redirect to="/auth" />
    } else {
        return (
            <div className={sharedStyles.fractalContainer}>
                <div data-testid={HEADER}>
                    <Header account />
                </div>
                <Row>
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
                    <Col sm={12} md={11} className="mt-12">
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
