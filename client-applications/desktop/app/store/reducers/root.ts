import { combineReducers } from "redux"
import { connectRouter } from "connected-react-router"
import { History } from "history"

import AuthReducer from "store/reducers/auth/reducer"
import ContainerReducer from "store/reducers/container/reducer"
import ClientReducer from "store/reducers/client/reducer"

const createRootReducer = (history: History) => {
    return combineReducers({
        router: connectRouter(history),
        AuthReducer: AuthReducer,
        ContainerReducer: ContainerReducer,
        ClientReducer: ClientReducer,
    })
}

export default createRootReducer
