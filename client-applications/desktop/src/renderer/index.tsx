import React, { useState, useEffect } from "react"
import { Switch, Route } from "react-router-dom"
import { Router } from "react-router"
import { chain } from "lodash"
import ReactDOM from "react-dom"

import Auth from "@app/renderer/pages/auth"
import Error from "@app/renderer/pages/error"
import {
    AuthErrorTitle,
    AuthErrorText,
    ContainerErrorTitle,
    ContainerErrorText,
    ProtocolErrorTitle,
    ProtocolErrorText,
    WindowHashAuth,
    WindowHashAuthError,
    WindowHashContainerError,
    WindowHashProtocolError,
    NavigationErrorTitle,
    NavigationErrorText,
} from "@app/utils/constants"

import { browserHistory } from "@app/utils/history"
import { useMainState } from "@app/utils/state"

// Electron has no way to pass data to a newly launched browser
// window. To avoid having to maintain multiple .html files for
// each kind of window, we pass a constant across to the renderer
// thread as a query parameter to determine which component
// should be rendered.

// If no query parameter match is found, we default to a
// generic navigation error window.
const show = chain(window.location.search.substring(1))
    .split("=")
    .chunk(2)
    .fromPairs()
    .get("show")
    .value()

const RootComponent = () => {
    const [_mainState, setMainState] = useMainState()

    const errorContinue = () =>
        setMainState({ errorRelaunchRequest: Date.now() })

    if (show === WindowHashAuth)
        return (
            <Router history={browserHistory}>
                <Switch>
                    <Route path="/" component={Auth} />
                </Switch>
            </Router>
        )
    if (show === WindowHashAuthError)
        return (
            <Error
                title={AuthErrorTitle}
                text={AuthErrorText}
                onClick={errorContinue}
            />
        )
    if (show === WindowHashContainerError)
        return (
            <Error
                title={ContainerErrorTitle}
                text={ContainerErrorText}
                onClick={errorContinue}
            />
        )
    if (show === WindowHashProtocolError)
        return (
            <Error
                title={ProtocolErrorTitle}
                text={ProtocolErrorText}
                onClick={errorContinue}
            />
        )
    return (
        <Error
            title={NavigationErrorTitle}
            text={NavigationErrorText}
            onClick={errorContinue}
        />
    )
}

// TODO: actually pass version number through IPC.
const WindowBackground = (props: any) => {
    return <div className="relative w-full h-full">
        <div className="bg-white absolute flex flex-col-reverse items-center w-full h-full"
             style={{zIndex: -10}}>
            <p className="font-body font-light text-gray-200 py-4">Version 1.0</p>
        </div>
        {props.children}
    </div>
}

ReactDOM.render(
    <WindowBackground>
        <RootComponent />
    </WindowBackground>
    , document.getElementById("root"))
