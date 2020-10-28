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
            // enforce structure of default but add in old information
            const mergeInto: any = deep_copy(DEFAULT)
            const userType: { [key: string]: any } = stateCopy.user // really annoying typescript syntax otherwise

            // if they are logged in then keep it that way
            if (
                state.user &&
                state.user.user_id &&
                state.user.accessToken &&
                state.user.refreshToken
            ) {
                Object.keys(state.user).forEach((key: string) => {
                    if (userType[key]) {
                        mergeInto.user[key] = userType[key]
                    }
                })
            }
            // authFlow should be set to init because they are not doing any auth actions
            // they should be forced to re-submit forgot requests and verify requests in each new session
            // since otherwise it will be complicated to only show the correct warning etc...
            // also I wonder if it could be bad for security
            return mergeInto
        default:
            return state
    }
}
