import React, { Fragment } from "react"
import { render } from "react-dom"
import { Provider } from "react-redux"
import { ConnectedRouter } from "connected-react-router"
import { AppContainer as ReactHotAppContainer } from "react-hot-loader"
import { configureStore, history } from "./store/configureStore"
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

import "./app.global.css"
import { config } from "shared/constants/config"

const store = configureStore()

const AppContainer = process.env.PLAIN_HMR ? Fragment : ReactHotAppContainer

// Set up Apollo GraphQL provider for https and wss (websocket)

console.log(config)

const httpLink = new HttpLink({
    uri: config.url.GRAPHQL_HTTP_URL,
    headers: {
        "x-hasura-access-key": config.keys.HASURA_ACCESS_KEY,
    },
})

const wsLink = new WebSocketLink({
    uri: config.url.GRAPHQL_WS_URL,
    options: {
        reconnect: true,
        connectionParams: {
            headers: {
                "x-hasura-admin-secret": config.keys.HASURA_ACCESS_KEY,
            },
        },
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

// apolloClient
//     .query({
//         query: gql`
//             query GetFeaturedApps {
//                 hardware_supported_app_images {
//                     app_id
//                     logo_url
//                 }
//             }
//         `,
//     })
//     .then((result) => console.log(result))

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
