/* eslint-disable import/prefer-default-export */
import React from "react"
import { render } from "@testing-library/react"
import { MockedProvider, MockedResponse } from "@apollo/client/testing"
import { Router, Switch, Route } from "react-router"
import { createMemoryHistory, MemoryHistory } from "history"
import { MainProvider } from "shared/context/mainContext"
import { Provider } from "react-redux"
import { applyMiddleware, createStore } from "redux"
import createSagaMiddleware from "redux-saga"
import { routerMiddleware } from "connected-react-router"
import ReduxPromise from "redux-promise"
import rootReducer from "store/reducers/root"
import rootSaga from "store/sagas/root"
import { routes } from "shared/constants/routes"
import { loadStripe } from "@stripe/stripe-js"
import { STRIPE_OPTIONS } from "shared/constants/stripe"
import { Elements } from "@stripe/react-stripe-js"
import { config } from "shared/constants/config"
import { UPDATE_ACCESS_TOKEN } from "shared/constants/graphql"

/**
 * A custom render to replace react-testing-library's render by wrapping it with all the necessary providers
 * to mock all the environment our website components need. These include a ApolloClient provider,
 * a mock store (with an initial state you can provide), a main context, and a router (necessary due to
 * the routes and switch in RootApp). Will allow us to easily test highly nested objects all of which depend
 * on the redux state as well as GraphQL. Useful for end-to-end testing.
 *
 * @param node the component we are rendering.
 * @param mocks the mock responses (check ApolloClient testing/mocking docs) and queries that we want to
 * be testing.
 * @param param2 Some default history.
 * @param initialState The initial state of the redux store.
 *
 * Note that leaving any of these blank is OK as long as they are not used, or used only to write.
 * Otherwise you must not leave them blank to insure correct behavior (i.e. emulating the real site).
 * If your component depends on nothing you can still use this and it will work as a regular render.
 */
export const customRender = (
    node: JSX.Element | null,
    initialState: Object = {},
    mocks: MockedResponse[] = [
        {
            request: {
                query: UPDATE_ACCESS_TOKEN,
                variables: {
                    loginToken: "loginToken",
                    accessToken: "accessToken",
                },
            },
            result: {
                data: {
                    affected_rows: 1,
                },
            },
        },
    ],
    history: MemoryHistory = createMemoryHistory({
        initialEntries: [routes.EMPTY],
    })
) => {
    const sagaMiddleware = createSagaMiddleware()
    const middleware = [routerMiddleware(history), ReduxPromise, sagaMiddleware]

    const store = createStore(
        rootReducer,
        initialState,
        applyMiddleware(...middleware)
    )
    sagaMiddleware.run(rootSaga)
    let STRIPE_PUBLIC_KEY: string = ""
    if (config.keys.STRIPE_PUBLIC_KEY == null) {
        // debugLog("Error: environment variable STRIPE_PUBLIC_KEY not set")
    } else {
        STRIPE_PUBLIC_KEY = config.keys.STRIPE_PUBLIC_KEY
    }
    const stripePromise = loadStripe(STRIPE_PUBLIC_KEY)

    return {
        history,
        ...render(
            <Router history={history}>
                <Provider store={store}>
                    <MockedProvider mocks={mocks} addTypename={false}>
                        <Elements
                            stripe={stripePromise}
                            options={STRIPE_OPTIONS}
                        >
                            <MainProvider>
                                <Switch>{node}</Switch>
                            </MainProvider>
                        </Elements>
                    </MockedProvider>
                </Provider>
            </Router>
        ),
    }
}
