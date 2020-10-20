import { DEFAULT } from "store/reducers/auth/default"

import * as PureAction from "store/actions/auth/pure"
import * as SharedAction from "store/actions/shared"

export default function (state = DEFAULT, action: any) {
    switch (action.type) {
        case PureAction.UPDATE_USER:
            console.log("reducer")
            const newUser = Object.assign(state.user, action.body)
            console.log(newUser)
            return {
                ...state,
                user: newUser,
            }
        case PureAction.UPDATE_AUTH_FLOW:
            console.log("reducer")
            const newAuthFlow = Object.assign(state.authFlow, action.body)
            console.log(newAuthFlow)
            return {
                ...state,
                authFlow: newAuthFlow,
            }
        case SharedAction.RESET_STATE:
            return DEFAULT
        default:
            return state
    }
}
