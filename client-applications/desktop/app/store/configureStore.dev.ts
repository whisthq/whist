import { createStore, applyMiddleware, compose } from "redux"
import createSagaMiddleware from "redux-saga"
import { createHashHistory } from "history"
import { routerMiddleware, routerActions } from "connected-react-router"
import { createLogger } from "redux-logger"
import createRootReducer from "store/reducers/index"
import * as MainActions from "store/actions/pure"
import { mainStateType } from "store/reducers/types"
import rootSaga from "store/sagas/index"

declare global {
    interface Window {
        __REDUX_DEVTOOLS_EXTENSION_COMPOSE__: (
            // eslint-disable-next-line @typescript-eslint/no-explicit-any
            obj: Record<string, any>
        ) => Function
    }
    interface NodeModule {
        hot?: {
            accept: (path: string, cb: () => void) => void
        }
    }
}

const history = createHashHistory()

const rootReducer = createRootReducer(history)

const configureStore = (initialState?: mainStateType) => {
    // Redux Configuration
    const middleware = []
    const enhancers = []

    // Thunk Middleware
    const sagaMiddleware = createSagaMiddleware()
    middleware.push(sagaMiddleware)

    // Logging Middleware
    const logger = createLogger({
        level: "info",
        collapsed: true,
    })

    // Skip redux logs in console during the tests
    if (process.env.NODE_ENV !== "test") {
        middleware.push(logger)
    }

    // Router Middleware
    const router = routerMiddleware(history)
    middleware.push(router)

    // Redux DevTools Configuration
    const actionCreators = {
        ...MainActions,
        ...routerActions,
    }
    // If Redux DevTools Extension is installed use it, otherwise use Redux compose
    /* eslint-disable no-underscore-dangle */
    const composeEnhancers = window.__REDUX_DEVTOOLS_EXTENSION_COMPOSE__
        ? window.__REDUX_DEVTOOLS_EXTENSION_COMPOSE__({
              // Options: http://extension.remotedev.io/docs/API/Arguments.html
              actionCreators,
          })
        : compose
    /* eslint-enable no-underscore-dangle */

    // Apply Middleware & Compose Enhancers
    enhancers.push(applyMiddleware(...middleware))
    const enhancer = composeEnhancers(...enhancers)

    // Create Store
    const store = createStore(rootReducer, initialState, enhancer)

    sagaMiddleware.run(rootSaga)

    if (module.hot) {
        module.hot.accept(
            "./reducers",
            // eslint-disable-next-line global-require
            () => store.replaceReducer(require("./reducers").default)
        )
    }

    return store
}

export default { configureStore, history }
