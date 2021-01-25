import { combineReducers } from "redux"
import { connectRouter } from "connected-react-router"
import { History } from "history"

import AuthReducer from "store/reducers/auth/reducer"
import ContainerReducer from "store/reducers/container/reducer"

export default function createRootReducer(history: History) {
    return combineReducers({
        router: connectRouter(history),
        AuthReducer: AuthReducer,
        ContainerReducer: ContainerReducer,
    })
}
