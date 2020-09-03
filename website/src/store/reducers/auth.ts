import { AUTH_DEFAULT } from "store/reducers/states";

import * as WaitlistAction from "store/actions/auth/waitlist";

export default function (state = AUTH_DEFAULT, action) {
    switch (action.type) {
        case WaitlistAction.INSERT_WAITLIST:
            return {
                ...state,
                user: state.user
                    ? {
                        ...state.user,
                        email: action.email,
                        name: action.name
                    }
                    : { email: action.email, name: action.name },
            };
        default:
            return state;
    }
}