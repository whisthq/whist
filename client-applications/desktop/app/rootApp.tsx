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
import { findDPI } from "pages/login/constants/helpers"
import { FractalRoute } from "shared/enums/navigation"

const RootApp = (props: any) => {
    const {
        launches,
        launchURL,
        os,
        dpi,
        candidateAccessToken,
        dispatch,
    } = props

    const [needsUpdate, setNeedsUpdate] = useState(false)
    const [updatePingReceived, setUpdatePingReceived] = useState(false)
    const [launched, setLaunched] = useState(false)

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
                    dispatch(
                        updateAuth({ candidateAccessToken: localAccessToken })
                    )
                } else {
                    dispatch(updateContainer({ launchURL: urlObj.hostname }))
                }
            }
        })

        // If already logged in, redirect to dashboard
        localAccessToken = storage.get("accessToken")
        if (localAccessToken) {
            dispatch(updateAuth({ candidateAccessToken: localAccessToken }))
        }
    }, [])

    useEffect(() => {
        if (!os || !dpi) {
            const dpi = findDPI()
            const os = require("os")
            dispatch(updateClient({ os: os.platform(), dpi: dpi }))
        }
    }, [os, dpi])

    // If there's an access token, validate it
    useEffect(() => {
        if (candidateAccessToken && candidateAccessToken !== "") {
            dispatch(validateAccessToken(candidateAccessToken))
        }
    }, [candidateAccessToken])

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
            history.push(FractalRoute.UPDATE)
        } else {
            if (!launchURL && props.username && props.accessToken) {
                updateAuth({ candidateAccessToken: "" })
                history.push(FractalRoute.DASHBOARD)
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
                        history.push(FractalRoute.LOADING)
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
                <Route path={FractalRoute.DASHBOARD} component={Dashboard} />
                <Route path={FractalRoute.LOADING} component={Loading} />
                <Route path={FractalRoute.LOGIN} component={Login} />
                <Route
                    path={FractalRoute.UPDATE}
                    render={(props) => (
                        <Update {...props} needsUpdate={needsUpdate} />
                    )}
                />
            </Switch>
        </div>
    )
}

const mapStateToProps = <T,>(state: T): T => {
    return {
        username: state.MainReducer.auth.username,
        candidateAccessToken: state.MainReducer.auth.candidateAccessToken,
        accessToken: state.MainReducer.auth.accessToken,
        refreshToken: state.MainReducer.auth.refreshToken,
        launches: state.MainReducer.container.launches,
        launchURL: state.MainReducer.container.launchURL,
        os: state.MainReducer.client.os,
        dpi: state.MainReducer.client.dpi,
    }
}

export default connect(mapStateToProps)(RootApp)
