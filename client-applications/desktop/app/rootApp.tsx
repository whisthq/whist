import React, { useEffect, useState } from "react"
import { connect } from "react-redux"
import { Route, Switch } from "react-router-dom"
import { useQuery } from "@apollo/client"

import Login from "pages/login/login"
import Loading from "pages/loading/loading"
import Dashboard from "pages/dashboard/dashboard"
import Update from "pages/update/update"

import { history } from "store/configureStore"
import { createContainer } from "store/actions/sideEffects"
import { updateClient, updateContainer } from "store/actions/pure"
import { setAWSRegion } from "shared/utils/exec"
import { checkActive, urlToApp } from "pages/login/constants/helpers"
import { GET_FEATURED_APPS } from "shared/constants/graphql"
import { debugLog } from "shared/utils/logging"

const RootApp = (props: any) => {
    const { username, accessToken, launches, launchURL, dispatch } = props

    const [needsUpdate, setNeedsUpdate] = useState(false)
    const [updatePingReceived, setUpdatePingReceived] = useState(false)
    const [launched, setLaunched] = useState(false)

    const { data } = useQuery(GET_FEATURED_APPS)

    const featuredAppData = data
        ? data.hardware_supported_app_images.filter(checkActive)
        : []

    useEffect(() => {
        const ipc = require("electron").ipcRenderer

        ipc.on("update", (_: any, update: any) => {
            setNeedsUpdate(update)
            setUpdatePingReceived(true)
        })

        ipc.on("customURL", (_: any, customURL: any) => {
            if (customURL && customURL.toString().includes("fractal://")) {
                customURL = customURL.split("fractal://")[1]
                dispatch(updateContainer({ launchURL: customURL }))
            }
        })
    }, [])

    useEffect(() => {
        if (
            updatePingReceived &&
            !needsUpdate &&
            launchURL &&
            username &&
            accessToken
        ) {
            dispatch(updateContainer({ launches: launches + 1 }))
            setLaunched(true)
        }
    }, [username, accessToken, launchURL, updatePingReceived, needsUpdate])

    useEffect(() => {
        if (needsUpdate && updatePingReceived) {
            history.push("/update")
        }
    }, [needsUpdate, updatePingReceived])

    useEffect(() => {
        if (launches === 1 && launched && launchURL && data) {
            setAWSRegion()
                .then((region) => {
                    return dispatch(updateClient({ region: region }))
                })
                .then(() => {
                    const { appID, url } = Object(
                        urlToApp(launchURL.toLowerCase(), featuredAppData)
                    )
                    dispatch(createContainer(appID, url))
                    return setLaunched(false)
                })
                .then(() => {
                    return history.push("/loading")
                })
                .catch((error) => {
                    "setAWSRegion() failed"
                })
        }
    }, [launches, launched, data, launchURL])

    return (
        <div>
            <Switch>
                <Route path="/dashboard" component={Dashboard} />
                <Route path="/loading" component={Loading} />
                <Route path="/" component={Login} />
                <Route
                    path="/update"
                    render={(props) => (
                        <Update {...props} needsUpdate={needsUpdate} />
                    )}
                />
            </Switch>
        </div>
    )
}

const mapStateToProps = (state: any) => {
    return {
        username: state.MainReducer.auth.username,
        accessToken: state.MainReducer.auth.accessToken,
        launches: state.MainReducer.container.launches,
        launchURL: state.MainReducer.container.launchURL,
    }
}

export default connect(mapStateToProps)(RootApp)
