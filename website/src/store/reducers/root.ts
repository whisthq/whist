import AuthReducer from "store/reducers/auth";
import { combineReducers } from "redux";

const reducers = combineReducers({
    AuthReducer: AuthReducer
});

const rootReducer = (state: any, action: any) => {
    if (action.type === "RESET_REDUX") {
        state = undefined;
    }

    return reducers(state, action);
};

export default rootReducer;