import { DEFAULT } from "store/reducers/auth/default"

import * as PureAction from "store/actions/auth/pure"
import * as SharedAction from "store/actions/shared"

export default function (state = DEFAULT, action: any) {
    switch (action.type) {
        case PureAction.UPDATE_USER:
            return {
                ...state,
                user: Object.assign(state.user, action.body),
            }
        case PureAction.UPDATE_AUTH_FLOW:
            return {
                ...state,
                authFlow: Object.assign(state.authFlow, action.body),
            }
        case SharedAction.RESET_STATE:
            return DEFAULT
        default:
            return state
    }
}
