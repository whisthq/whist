import React, { useEffect } from "react"
import { connect } from "react-redux"
import { Route, Switch } from "react-router-dom"

import Login from "pages/login/login"
import Loading from "pages/loading/loading"
import Dashboard from "pages/dashboard/dashboard"

const RootApp = (props: any) => {
    return (
        <div>
            <Switch>
                <Route path="/dashboard" component={Dashboard} />
                <Route path="/loading" component={Loading} />
                <Route path="/" component={Login} />
            </Switch>
        </div>
    )
}

export default RootApp
