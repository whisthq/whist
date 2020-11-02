import React from "react"
import ReactDOM from "react-dom"
import { Provider } from "react-redux"
import { applyMiddleware, createStore } from "redux"
import createSagaMiddleware from "redux-saga"
import { persistStore, persistReducer } from "redux-persist"
import autoMergeLevel2 from "redux-persist/lib/stateReconciler/autoMergeLevel2"
import { routerMiddleware } from "connected-react-router"
import { Router } from "react-router"
import { PersistGate } from "redux-persist/integration/react"
import { composeWithDevTools } from "redux-devtools-extension"
import ReduxPromise from "redux-promise"
import storage from "redux-persist/lib/storage"
import rootSaga from "store/sagas/root"
import * as Sentry from "@sentry/react"
import { ApolloProvider } from "@apollo/react-hooks"
import { ApolloClient, InMemoryCache, HttpLink, split } from "@apollo/client"
import { getMainDefinition } from "@apollo/client/utilities"
import { WebSocketLink } from "@apollo/client/link/ws"

import history from "shared/utils/history"
import { MainProvider } from "shared/context/mainContext"
import { config } from "shared/constants/config"
import rootReducer from "store/reducers/root"
import * as serviceWorker from "serviceWorker"

import "styles/shared.css"
import "bootstrap/dist/css/bootstrap.min.css"

import RootApp from "rootApp"

Sentry.init({
    dsn:
        "https://9a25b78ce37b4f7db2ff1a4952c1e3a8@o400459.ingest.sentry.io/5394481",
    environment: config.sentry_env,
    release: "website@" + process.env.REACT_APP_VERSION,
})

const sentryReduxEnhancer = Sentry.createReduxEnhancer({})

const persistConfig = {
    key: "rootKey",
    storage,
    stateReconciler: autoMergeLevel2, // allows you to add new keys
}

const sagaMiddleware = createSagaMiddleware()
const persistedReducer = persistReducer(persistConfig, rootReducer)

let middleware = [routerMiddleware(history), ReduxPromise, sagaMiddleware]

const store = createStore(
    persistedReducer,
    composeWithDevTools(applyMiddleware(...middleware), sentryReduxEnhancer)
)

const persistor = persistStore(store)

sagaMiddleware.run(rootSaga)

// Set up Apollo GraphQL provider for https and wss (websocket)

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

ReactDOM.render(
    <React.StrictMode>
        <Router history={history}>
            <Provider store={store}>
                <PersistGate loading={null} persistor={persistor}>
                    <ApolloProvider client={apolloClient}>
                        <MainProvider>
                            <RootApp />
                        </MainProvider>
                    </ApolloProvider>
                </PersistGate>
            </Provider>
        </Router>
    </React.StrictMode>,
    document.getElementById("root")
)

// If you want your app to work offline and load faster, you can change
// unregister() to register() below. Note this comes with some pitfalls.
// Learn more about service workers: https://bit.ly/CRA-PWA
serviceWorker.unregister()
