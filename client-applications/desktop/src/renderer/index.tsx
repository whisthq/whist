import React from "react"
import { Switch, Route } from "react-router-dom"
import { Router } from "react-router"
import ReactDOM from "react-dom"

import Auth from "@app/renderer/pages/auth"

import { browserHistory } from "@app/utils/history"

const RootComponent = () => (
    <>
        <Router history={browserHistory}>
            <Switch>
                <Route exact path="/" component={Auth} />
                <Route path="/auth" component={Auth} />
            </Switch>
        </Router>
    </>
)

ReactDOM.render(<RootComponent />, document.getElementById("root"))
