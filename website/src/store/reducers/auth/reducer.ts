import { DEFAULT } from "store/reducers/auth/default"

import * as PureAction from "store/actions/auth/pure"
import * as SharedAction from "store/actions/shared"

import { deep_copy } from "shared/utils/reducerHelpers"

export default function (state = DEFAULT, action: any) {
    var stateCopy = deep_copy(state)
    switch (action.type) {
        case PureAction.UPDATE_USER:
            return {
                ...stateCopy,
                user: Object.assign(stateCopy.user, action.body),
            }
        case PureAction.UPDATE_AUTH_FLOW:
            return {
                ...stateCopy,
                authFlow: Object.assign(stateCopy.authFlow, action.body),
            }
        case PureAction.RESET_USER:
            return {
                ...stateCopy,
                user: DEFAULT.user,
            }
        case SharedAction.RESET_STATE:
            return DEFAULT
        case SharedAction.REFRESH_STATE:
            return DEFAULT
        default:
            return state
    }
}
