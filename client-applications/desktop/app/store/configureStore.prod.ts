import { createStore, applyMiddleware } from "redux"
import createSagaMiddleware from "redux-saga"
import createRootReducer from "store/reducers/index"
import { Store, mainStateType } from "store/reducers/types"
import rootSaga from "store/sagas/index"
import { history } from "./history"

const rootReducer = createRootReducer(history)
const sagaMiddleware = createSagaMiddleware()
const enhancer = applyMiddleware(sagaMiddleware)

function configureStore(initialState?: mainStateType): Store {
    const store = createStore(rootReducer, initialState, enhancer)
    sagaMiddleware.run(rootSaga)
    return store
}

export default { configureStore }
