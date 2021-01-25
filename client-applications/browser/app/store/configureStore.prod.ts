import { createStore, applyMiddleware } from "redux"
import createSagaMiddleware from "redux-saga"
import createRootReducer from "store/reducers/root"
import { Store } from "shared/types/redux"
import rootSaga from "store/sagas/root"
import { history } from "./history"

const rootReducer = createRootReducer(history)
const sagaMiddleware = createSagaMiddleware()
const enhancer = applyMiddleware(sagaMiddleware)

function configureStore(initialState?: any): Store {
    const store = createStore(rootReducer, initialState, enhancer)
    sagaMiddleware.run(rootSaga)
    return store
}

export default { configureStore }
