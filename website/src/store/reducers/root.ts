import AuthReducer from "store/reducers/auth/reducer"
import DashboardReducer from "store/reducers/dashboard/reducer"

import { combineReducers } from "redux"

const reducers = combineReducers({
    AuthReducer: AuthReducer,
    DashboardReducer: DashboardReducer,
})

const rootReducer = (state: any, action: any) => {
    if (action.type === "RESET_REDUX") {
        state = undefined
    }

    return reducers(state, action)
}

export default rootReducer
