import React, { useEffect, useState } from "react"
import { connect } from "react-redux"
import { Route, Switch } from "react-router-dom"
import { useQuery } from "@apollo/client"

import Login from "pages/login/login"
import Loading from "pages/loading/loading"
import Dashboard from "pages/dashboard/dashboard"
import Update from "pages/update/update"

import { history } from "store/configureStore"
import { createContainer, validateAccessToken } from "store/actions/sideEffects"
import { updateClient, updateContainer, updateAuth } from "store/actions/pure"
import { setAWSRegion } from "shared/utils/exec"
import { checkActive, urlToApp } from "pages/login/constants/helpers"
import { GET_FEATURED_APPS } from "shared/constants/graphql"

const RootApp = (props: any) => {
    const { launches, launchURL, dispatch } = props

    const [needsUpdate, setNeedsUpdate] = useState(false)
    const [updatePingReceived, setUpdatePingReceived] = useState(false)
    const [launched, setLaunched] = useState(false)
    const [accessToken, setAccessToken] = useState("")

    const { data } = useQuery(GET_FEATURED_APPS)

    const featuredAppData = data
        ? data.hardware_supported_app_images.filter(checkActive)
        : []

    const Store = require("electron-store")
    const storage = new Store()

    useEffect(() => {
        const ipc = require("electron").ipcRenderer
        var localAccessToken: string | null = null

        // Update listener
        ipc.on("update", (_: any, update: any) => {
            setNeedsUpdate(update)
            setUpdatePingReceived(true)
        })

        // Custom URL listener
        ipc.on("customURL", (_: any, customURL: string) => {
            if (customURL && customURL.toString().includes("fractal://")) {
                customURL = "fractal://" + customURL.split("fractal://")[1]
                // Convert URL to URL object so it can be parsed
                const urlObj = new URL(customURL.toString())
                urlObj.protocol = "https"

                // Check to see if this is an auth request
                localAccessToken = urlObj.searchParams.get("accessToken")
                if (localAccessToken) {
                    setAccessToken(localAccessToken)
                    dispatch(updateContainer({ launchURL: null }))
                } else {
                    dispatch(updateContainer({ launchURL: urlObj.hostname }))
                }
            }
        })

        // If already logged in, redirect to dashboard
        localAccessToken = storage.get("accessToken")
        if (localAccessToken) {
            setAccessToken(localAccessToken)
        }
    }, [])

    // If there's an access token, validate it
    useEffect(() => {
        if (accessToken && accessToken !== "") {
            dispatch(validateAccessToken(accessToken))
        }
    }, [accessToken])

    // If does not need update, logged in and ready to launch
    useEffect(() => {
        if (
            updatePingReceived &&
            !needsUpdate &&
            launchURL &&
            props.username &&
            props.accessToken
        ) {
            dispatch(updateContainer({ launches: launches + 1 }))
            setLaunched(true)
        }
    }, [
        props.username,
        props.accessToken,
        launchURL,
        updatePingReceived,
        needsUpdate,
    ])

    // If there's an update, redirect to update screen
    useEffect(() => {
        if (needsUpdate && updatePingReceived) {
            history.push("/update")
        } else {
            if (!launchURL && props.username && props.accessToken) {
                setAccessToken("")
                history.push("/dashboard")
            } else if (
                launches === 1 &&
                launched &&
                launchURL &&
                data &&
                props.username &&
                props.accessToken
            ) {
                setAWSRegion()
                    .then((region) => {
                        dispatch(updateClient({ region: region }))
                    })
                    .then(() => {
                        const { app_id, url } = Object(
                            urlToApp(launchURL.toLowerCase(), featuredAppData)
                        )
                        dispatch(createContainer(app_id, url))
                        setLaunched(false)
                    })
                    .then(() => {
                        history.push("/loading")
                    })
            }
        }
    }, [
        needsUpdate,
        updatePingReceived,
        launchURL,
        launches,
        props.username,
        props.accessToken,
        data,
    ])

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
        refreshToken: state.MainReducer.auth.refreshToken,
        launches: state.MainReducer.container.launches,
        launchURL: state.MainReducer.container.launchURL,
    }
}

export default connect(mapStateToProps)(RootApp)
