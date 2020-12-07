import React, { Fragment } from "react"
import { render } from "react-dom"
import { Provider } from "react-redux"
import { ConnectedRouter } from "connected-react-router"
import { AppContainer as ReactHotAppContainer } from "react-hot-loader"
import {
    ApolloProvider,
    ApolloClient,
    InMemoryCache,
    HttpLink,
    split,
} from "@apollo/client"
import { getMainDefinition } from "@apollo/client/utilities"
import { WebSocketLink } from "@apollo/client/link/ws"

import RootApp from "rootApp"

import "app.global.css"
import { config } from "shared/constants/config"
import configureStore from "store/configureStore"
import { history } from "store/history"

import * as Sentry from "@sentry/electron";
if (process.env.NODE_ENV === "production"){
    Sentry.init({
        dsn: "https://5b0accb25f3341d280bb76f08775efe1@o400459.ingest.sentry.io/5412323",
    });
}

const store = configureStore()

const AppContainer = process.env.PLAIN_HMR ? Fragment : ReactHotAppContainer

// Set up Apollo GraphQL provider for https and wss (websocket)

const httpLink = new HttpLink({
    uri: config.url.GRAPHQL_HTTP_URL,
})

const wsLink = new WebSocketLink({
    uri: config.url.GRAPHQL_WS_URL,
    options: {
        reconnect: true,
    },
})

const splitLink = split(
    ({ query }) => {
        const definition = getMainDefinition(query)
        return (
            definition.kind === "OperationDefinition" &&
            definition.operation === "subscription"
        )
    },
    wsLink,
    httpLink
)

const apolloClient = new ApolloClient({
    link: splitLink,
    cache: new InMemoryCache(),
})

document.addEventListener("DOMContentLoaded", () =>
    render(
        <AppContainer>
            <Provider store={store}>
                <ConnectedRouter history={history}>
                    <ApolloProvider client={apolloClient}>
                        <RootApp />
                    </ApolloProvider>
                </ConnectedRouter>
            </Provider>
        </AppContainer>,
        document.getElementById("root")
    )
)
