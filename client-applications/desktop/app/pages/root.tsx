import React from "react"
import { connect } from "react-redux"
import { Route, Switch } from "react-router-dom"

import LoadingPage from "pages/loading/loading"
import LauncherPage from "pages/launcher/launcher"
import LoginPage from "pages/login/login"
import UpdatePage from "pages/update/update"

import TitleBar from "shared/components/titleBar"

import { FractalRoute } from "shared/types/navigation"

const Root = () => {
    /*
        Highest level component, containers React Router and title bar

        Arguments: none
    */

    return (
        <div>
            <TitleBar />
            <Switch>
                <Route path={FractalRoute.LAUNCHER} component={LauncherPage} />
                <Route path={FractalRoute.LOGIN} component={LoginPage} />
                <Route path={FractalRoute.UPDATE} component={UpdatePage} />
                <Route path={FractalRoute.LOADING} component={LoadingPage} />
            </Switch>
        </div>
    )
}

// Keeping here for future use
const mapStateToProps = () => {
    return {}
}

export default connect(mapStateToProps)(Root)
