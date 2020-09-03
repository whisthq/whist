import LandingReducer from 'store/reducers/landing'
import { combineReducers } from 'redux'

const reducers = combineReducers({
    LandingReducer: LandingReducer,
})

const rootReducer = (state: any, action: any) => {
    if (action.type === 'RESET_REDUX') {
        state = undefined
    }

    return reducers(state, action)
}

export default rootReducer
