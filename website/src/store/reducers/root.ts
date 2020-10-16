import AuthReducer from "store/reducers/auth/reducer"
import WaitlistReducer from "store/reducers/waitlist/reducer"

import { combineReducers } from "redux"

const reducers = combineReducers({
    AuthReducer: AuthReducer,
    WaitlistReducer: WaitlistReducer,
})

const rootReducer = (state: any, action: any) => {
    if (action.type === "RESET_REDUX") {
        state = undefined
    }

    return reducers(state, action)
}

export default rootReducer
