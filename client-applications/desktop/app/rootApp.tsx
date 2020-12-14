import React, { useEffect, useState, Dispatch } from "react"
import { connect } from "react-redux"
import { Route, Switch } from "react-router-dom"
import { useQuery } from "@apollo/client"

import Login from "pages/login/login"
import Loading from "pages/loading/loading"
import Dashboard from "pages/dashboard/dashboard"
import Update from "pages/update/update"
import Welcome from "pages/onboard/welcome"
import Apps from "pages/onboard/apps"
import Installing from "pages/onboard/installing"
import Storage from "pages/onboard/storage"

import { history } from "store/history"
import { createContainer, validateAccessToken } from "store/actions/sideEffects"
import {
    updateClient,
    updateContainer,
    updateAuth,
    updateApps,
} from "store/actions/pure"
import { setAWSRegion } from "shared/utils/files/exec"
import { checkActive, urlToApp, findDPI } from "pages/login/constants/helpers"
import { GET_FEATURED_APPS } from "shared/constants/graphql"

// import { OperatingSystem } from "shared/types/client"
import { FractalRoute } from "shared/types/navigation"
import { FractalAuthCache } from "shared/types/cache"

const RootApp = (props: {
    launches: number
    launchURL: string
    clientOS: string
    dpi: number
    username: string
    accessToken: string
    connectedApps: string[]
    dispatch: Dispatch<any>
}) => {
    const {
        launches,
        launchURL,
        clientOS,
        dpi,
        connectedApps,
        dispatch,
    } = props

    const [needsUpdate, setNeedsUpdate] = useState(false)
    const [updatePingReceived, setUpdatePingReceived] = useState(false)
    const [launched, setLaunched] = useState(false)
    const [accessTokenRetrieved, setAccessTokenRetrieved] = useState(false)
    const [urlReceived, setUrlReceived] = useState(false)

    const { data } = useQuery(GET_FEATURED_APPS)

    const featuredAppData = data
        ? data.hardware_supported_app_images.filter(checkActive)
        : []

    const Store = require("electron-store")
    const storage = new Store()

    useEffect(() => {
        const ipc = require("electron").ipcRenderer
        let localAccessToken: string | null = null

        // Update listener
        ipc.on("update", (_: any, update: boolean) => {
            setNeedsUpdate(update)
            setUpdatePingReceived(true)
        })

        // Custom URL listener
        ipc.on("customURL", (_: any, customURL: string) => {
            if (customURL && customURL.toString().includes("fractal://")) {
                customURL = `fractal://${customURL.split("fractal://")[1]}`
                // Convert URL to URL object so it can be parsed
                const urlObj = new URL(customURL.toString())
                urlObj.protocol = "https"

                // Check to see if this is an auth request or an external app authentication
                localAccessToken = urlObj.searchParams.get("accessToken")
                const authenticatedApp = urlObj.searchParams.get(
                    "successfully_authenticated"
                )
                if (localAccessToken) {
                    dispatch(
                        updateAuth({
                            candidateAccessToken: localAccessToken,
                        })
                    )
                    dispatch(validateAccessToken(localAccessToken))
                } else if (authenticatedApp) {
                    if (connectedApps.indexOf(authenticatedApp) === -1) {
                        const newConnectedApps = Object.assign(
                            [],
                            connectedApps
                        )
                        newConnectedApps.push(authenticatedApp)
                        dispatch(updateApps({ connected: newConnectedApps }))
                    }
                } else {
                    dispatch(updateContainer({ launchURL: urlObj.hostname }))
                }
            }
        })

        // If already logged in, redirect to dashboard
        localAccessToken = storage.get(FractalAuthCache.ACCESS_TOKEN)
        if (localAccessToken) {
            dispatch(updateAuth({ candidateAccessToken: localAccessToken }))
            dispatch(validateAccessToken(localAccessToken))
        }

        setAccessTokenRetrieved(true)
    }, [])

    useEffect(() => {
        if (!clientOS || !dpi) {
            dispatch(
                updateClient({
                    clientOS: require("os").platform(),
                    dpi: findDPI(),
                })
            )
        }
    }, [clientOS, dpi])

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

    // Wait two seconds to allow for fractal:// to be detected
    useEffect(() => {
        setTimeout(() => {
            setUrlReceived(true)
        }, 3000)
    }, [])

    // If there's an update, redirect to update screen
    useEffect(() => {
        if (needsUpdate && updatePingReceived) {
            history.push(FractalRoute.UPDATE)
        } else if (!launchURL && props.username && props.accessToken) {
            updateAuth({ candidateAccessToken: "" })
            const onboarded = storage.get(FractalAuthCache.ONBOARDED)
            if (onboarded) {
                history.push(FractalRoute.DASHBOARD)
            } else {
                history.push(FractalRoute.ONBOARD_WELCOME)
            }
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
                    return null
                })
                .then(() => {
                    const { appID, url } = Object(
                        urlToApp(launchURL.toLowerCase(), featuredAppData)
                    )
                    dispatch(createContainer(appID, url))
                    setLaunched(false)
                    return null
                })
                .then(() => {
                    history.push(FractalRoute.LOADING)
                    return null
                })
                .catch((err) => {
                    throw err
                })
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
                <Route
                    path={FractalRoute.UPDATE}
                    render={() => <Update needsUpdate={needsUpdate} />}
                />
                <Route
                    path={FractalRoute.ONBOARD_WELCOME}
                    component={Welcome}
                />
                <Route path={FractalRoute.ONBOARD_APPS} component={Apps} />
                <Route
                    path={FractalRoute.ONBOARD_INSTALLING}
                    component={Installing}
                />
                <Route
                    path={FractalRoute.ONBOARD_STORAGE}
                    component={Storage}
                />
                <Route
                    path={FractalRoute.LOGIN}
                    component={() => (
                        <Login loaded={accessTokenRetrieved && urlReceived} />
                    )}
                />
            </Switch>
        </div>
    )
}

const mapStateToProps = (state: {
    MainReducer: {
        auth: {
            username: string
            accessToken: string
            refreshToken: string
        }
        container: {
            launches: number
            launchURL: string
        }
        client: {
            clientOS: string
            dpi: number
        }
        apps: {
            connected: string[]
        }
    }
}) => {
    return {
        username: state.MainReducer.auth.username,
        accessToken: state.MainReducer.auth.accessToken,
        refreshToken: state.MainReducer.auth.refreshToken,
        launches: state.MainReducer.container.launches,
        launchURL: state.MainReducer.container.launchURL,
        clientOS: state.MainReducer.client.clientOS,
        dpi: state.MainReducer.client.dpi,
        connectedApps: state.MainReducer.apps.connected,
    }
}

export default connect(mapStateToProps)(RootApp)
