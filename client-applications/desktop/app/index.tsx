import React, { Fragment } from "react"
import { render } from "react-dom"
import { Provider } from "react-redux"
import { ConnectedRouter } from "connected-react-router"
import { AppContainer as ReactHotAppContainer } from "react-hot-loader"

import { ApolloProvider } from "@apollo/client"
import ApolloClient from "configureApolloClient"

import RootApp from "rootApp"

import "app.global.css"
import configureStore from "store/configureStore"
import { history } from "store/history"

// set up Sentry - import proper init based on running thread
const { init } =
    process.type === "browser"
        ? require("@sentry/electron/dist/main")
        : require("@sentry/electron/dist/renderer")

if (process.env.NODE_ENV === "production") {
    init({
        dsn:
            "https://5b0accb25f3341d280bb76f08775efe1@o400459.ingest.sentry.io/5412323",
        environment: config.sentryEnv,
        release: "client-applications@1.0.0", // TODO: make this the real release version
    })
}

const store = configureStore()

const AppContainer = process.env.PLAIN_HMR ? Fragment : ReactHotAppContainer

document.addEventListener("DOMContentLoaded", () =>
    render(
        <AppContainer>
            <Provider store={store}>
                <ConnectedRouter history={history}>
                    <ApolloProvider client={ApolloClient}>
                        <RootApp />
                    </ApolloProvider>
                </ConnectedRouter>
            </Provider>
        </AppContainer>,
        document.getElementById("root")
    )
)
