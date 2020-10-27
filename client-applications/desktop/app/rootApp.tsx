import React, { useEffect, useState } from "react"
import { connect } from "react-redux"
import { Route, Switch } from "react-router-dom"

import Login from "pages/login/login"
import Loading from "pages/loading/loading"
import Dashboard from "pages/dashboard/dashboard"

import { fetchContainer } from "store/actions/sideEffects"

const RootApp = (props: any) => {
    const { dispatch } = props
    const [launchImmediately, setLaunchImmediately] = useState(false)

    useEffect(() => {
        const ipc = require("electron").ipcRenderer
        ipc.on("customURL", (_: any, customURL: any) => {
            if (customURL && customURL.toString().includes("fractal://")) {
                console.log("launch immediately root app")
                setLaunchImmediately(true)
            }
        })
    }, [])

    return (
        <div>
            <Switch>
                <Route path="/dashboard" component={Dashboard} />
                <Route path="/loading" component={Loading} />
                <Route path = "/" render={(props) => (
                    <Login {...props} launchImmediately={launchImmediately} />
                )}/>
            </Switch>
        </div>
    )
}

const mapStateToProps = (state: any) => {
    return {
        username: state.MainReducer.auth.username,
    }
}

export default connect(mapStateToProps)(RootApp)
