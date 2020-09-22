import MainReducer from "store/reducers/main"
import { combineReducers } from "redux"

const reducers = combineReducers({
    MainReducer: MainReducer,
})

const rootReducer = (state: any, action: any) => {
    if (action.type === "RESET_REDUX") {
        state = undefined
    }

    return reducers(state, action)
}

export default rootReducer
