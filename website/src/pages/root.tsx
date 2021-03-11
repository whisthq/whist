import React, { useEffect, Dispatch } from "react"
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

import About from "pages/homepage/about/about"
import TermsOfService from "pages/legal/tos"
import Cookies from "pages/legal/cookies"
import Privacy from "pages/legal/privacy"
import Auth from "pages/auth/auth"
import Verify from "pages/auth/verify/verify"
import Reset from "pages/auth/reset/reset"
import Dashboard from "pages/dashboard/dashboard"
import AuthCallback from "pages/auth/callback/callback"
import Products from "pages/homepage/products/products"

import routes from "shared/constants/routes"
import withTracker from "shared/utils/withTracker"
import { config } from "shared/constants/config"
import { User } from "shared/types/reducers"

import * as SharedAction from "store/actions/shared"

const RootApp = (props: {
    accessToken: string | null | undefined
    dispatch: Dispatch<any>
}) => {
    /*
        Highest-level React component, contains router and ApolloClient

        Arguments:
            accessToken(string): Access token, if any
    */

    const { accessToken, dispatch } = props

    const refreshState = () => {
        dispatch(SharedAction.refreshState())
    }

    const createApolloClient = (token: string | undefined | null) => {
        if (!token) {
            token = ""
        }

        const httpLink = new HttpLink({
            uri: config.url.GRAPHQL_HTTP_URL,
        })

        const wsLink = new WebSocketLink({
            uri: config.url.GRAPHQL_WS_URL,
            options: {
                reconnect: true,
                connectionParams: {
                    headers: {
                        Authorization: `Bearer ${token}`,
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

    const apolloClient = createApolloClient(accessToken)

    // Will refresh the Redux state on component mount in case of Redux changes
    useEffect(() => {
        refreshState()
        // eslint-disable-next-line react-hooks/exhaustive-deps
    }, [])

    return (
        <ApolloProvider client={apolloClient}>
            <Switch>
                <Route
                    exact
                    path={routes.ABOUT}
                    component={withTracker(About)}
                />
                <Route
                    exact
                    path={routes.COOKIES}
                    component={withTracker(Cookies)}
                />
                <Route
                    exact
                    path={routes.PRIVACY}
                    component={withTracker(Privacy)}
                />
                <Route
                    exact
                    path={routes.TOS}
                    component={withTracker(TermsOfService)}
                />
                <Route path={routes.AUTH} component={withTracker(Auth)} />
                <Route
                    exact
                    path={routes.VERIFY}
                    component={withTracker(Verify)}
                />
                <Route
                    exact
                    path={routes.RESET}
                    component={withTracker(Reset)}
                />
                <Route
                    exact
                    path={routes.AUTH_CALLBACK}
                    component={withTracker(AuthCallback)}
                />
                <Route
                    path={routes.DASHBOARD}
                    component={withTracker(Dashboard)}
                />
                <Route
                    exact
                    path={routes.LANDING}
                    component={withTracker(Products)}
                />
            </Switch>
        </ApolloProvider>
    )
}

const mapStateToProps = (state: {
    AuthReducer: {
        user: User
    }
}) => {
    return {
        accessToken: state.AuthReducer.user.accessToken,
    }
}

export default connect(mapStateToProps)(RootApp)
