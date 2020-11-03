import React, { useEffect, useState } from "react"
import { connect } from "react-redux"
import { Route, Switch } from "react-router-dom"

import Login from "pages/login/login"
import Loading from "pages/loading/loading"
import Dashboard from "pages/dashboard/dashboard"

const RootApp = (props: any) => {
    const [launchImmediately, setLaunchImmediately] = useState(false)
    const [launchURL, setLaunchURL] = useState("")

    useEffect(() => {
        const ipc = require("electron").ipcRenderer
        ipc.on("customURL", (_: any, customURL: any) => {
            if (customURL && customURL.toString().includes("fractal://")) {
                customURL.replace("fractal://", "")
                setLaunchImmediately(true)
                setLaunchURL(customURL)
            }
        })
    }, [])

    return (
        <div>
            <Switch>
                <Route path="/dashboard" component={Dashboard} />
                <Route path="/loading" component={Loading} />
                <Route
                    path="/"
                    render={(props) => (
                        <Login
                            {...props}
                            launchImmediately={launchImmediately}
                            launchURL={launchURL}
                        />
                    )}
                />
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
