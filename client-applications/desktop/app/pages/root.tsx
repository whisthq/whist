import React from "react"
import { connect } from "react-redux"
import { Route, Switch } from "react-router-dom"
import {
    ApolloProvider,
    ApolloClient,
    InMemoryCache,
    HttpLink,
    split,
} from "@apollo/client"
import { getMainDefinition } from "@apollo/client/utilities"
import { WebSocketLink } from "@apollo/client/link/ws"

import LoadingPage from "pages/loading/loading"
import LauncherPage from "pages/launcher/launcher"
import LoginPage from "pages/login/login"
import UpdatePage from "pages/update/update"
import PaymentPage from "pages/payment/payment"

import TitleBar from "shared/components/titleBar"
import { config } from "shared/constants/config"
import { FractalRoute } from "shared/types/navigation"

const Root = (props: { loginToken: string }) => {
    /*
        Highest level component, containers React Router and title bar

        Arguments:
            loginToken: one time use token for retrieving the user access token
    */

    const { loginToken } = props

    // Set up Apollo GraphQL provider for https and wss (websocket)
    const createApolloClient = (tempLoginToken: string) => {
        const httpLink = new HttpLink({
            uri: config.url.GRAPHQL_HTTP_URL,
        })

        const wsLink = new WebSocketLink({
            uri: config.url.GRAPHQL_WS_URL,
            options: {
                reconnect: true,
                connectionParams: {
                    headers: {
                        Login: tempLoginToken,
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
        return new ApolloClient({
            link: splitLink,
            cache: new InMemoryCache(),
        })
    }

    const apolloClient = createApolloClient(loginToken)

    return (
        <ApolloProvider client={apolloClient}>
            <div>
                <TitleBar />
                <Switch>
                    <Route
                        path={FractalRoute.LAUNCHER}
                        component={LauncherPage}
                    />
                    <Route path={FractalRoute.LOGIN} component={LoginPage} />
                    <Route path={FractalRoute.UPDATE} component={UpdatePage} />
                    <Route
                        path={FractalRoute.PAYMENT}
                        component={PaymentPage}
                    />
                    <Route
                        path={FractalRoute.LOADING}
                        component={LoadingPage}
                    />
                </Switch>
            </div>
        </ApolloProvider>
    )
}

// Keeping here for future use
const mapStateToProps = (state: {
    AuthReducer: { authFlow: { loginToken: string } }
}) => {
    return {
        loginToken: state.AuthReducer.authFlow.loginToken,
    }
}

export default connect(mapStateToProps)(Root)
